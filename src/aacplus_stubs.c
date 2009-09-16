/*
 * OCaml bindings for libaacplus
 *
 * Copyright 2005-2006 Savonet team
 *
 * This file is part of ocaml-aacplus.
 *
 * ocaml-aacplus is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * ocaml-aacplus is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with ocaml-aacplus; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

 /* OCaml bindings for the libaacplus library. */

#include <caml/memory.h>
#include <caml/alloc.h>
#include <caml/fail.h>
#include <caml/callback.h>
#include <caml/custom.h>
#include <caml/signals.h>

#include <string.h>

#include <libaacplus/sbr_main.h>
#include <libaacplus/aacenc.h>
#include <libaacplus/adts.h>
#include <libaacplus/cfftn.h>
#include <libaacplus/resampler.h>

#define CORE_DELAY   (1600)
/* ((1600 (core codec)*2 (multi rate) + 6*64 (sbr dec delay) - 2048 (sbr enc delay) + magic */
#define INPUT_DELAY  ((CORE_DELAY)*2 +6*64-2048+1)     
/* the additional max resampler filter delay (source fs) */
#define MAX_DS_FILTER_DELAY 16
/* (96-64) makes AAC still some 64 core samples too early wrt SBR ... maybe -32 would be even more correct, 
 * but 1024-32 would need additional SBR bitstream delay by one frame */
#define CORE_INPUT_OFFSET_PS (0)  

typedef struct AacpAudioContext {
    struct AAC_ENCODER *aacEnc;
    HANDLE_SBR_ENCODER hEnvEnc;

    AACENC_CONFIG     config;
    sbrConfiguration sbrConfig;
    
    IIR21_RESAMPLER IIR21_reSampler[MAX_CHANNELS];
    float inputBuffer[(AACENC_BLOCKSIZE*2 + MAX_DS_FILTER_DELAY + INPUT_DELAY)*MAX_CHANNELS];
    char frame[(6144/8)*MAX_CHANNELS+ADTS_HEADER_SIZE];  
  
    int nChannelsAAC, nChannelsSBR;
    unsigned int sampleRateAAC;

    unsigned int numAncDataBytes;
    int useParametricStereo;
    int coreWriteOffset;
    int envReadOffset;
    int writeOffset;
    
    unsigned char ancDataBytes[MAX_PAYLOAD_SIZE];
    char adtsDataBytes[ADTS_HEADER_SIZE];
    int adtsOffset;
} AacpAudioContext;

#define Aac_env_val(v) (*((AacpAudioContext**)Data_custom_val(v)))

static void finalize_aac_env(value c)
{
  AacpAudioContext *s = Aac_env_val(c);
  AacEncClose(s->aacEnc);
  EnvClose(s->hEnvEnc);
  free(s);
}

static struct custom_operations aac_env_ops =
{
  "ocaml_aacplus_aac_env",
  finalize_aac_env,
  custom_compare_default,
  custom_hash_default,
  custom_serialize_default,
  custom_deserialize_default
};

CAMLprim value ocaml_aacplus_init()
{
  CAMLparam0();
  init_plans(); 
  CAMLreturn(Val_unit);
}

CAMLprim value ocaml_aacplus_destroy()
{
  CAMLparam0();
  destroy_plans();
  CAMLreturn(Val_unit);
}

CAMLprim value ocaml_aacplus_block_size(value aac_env)
{
  CAMLparam1(aac_env);
  AacpAudioContext *s = Aac_env_val(aac_env);
  /* String size: 2 bytes per elements. */
  CAMLreturn(Val_int(AACENC_BLOCKSIZE*2*2*(s->nChannelsSBR)));
}

CAMLprim value ocaml_aacplus_init_enc(value chans, value samplerate, value bitrate)
{
  CAMLparam0();
  CAMLlocal1(ans);

  int channels = Int_val(chans);
  int sample_rate = Int_val(samplerate);
  int bit_rate = Int_val(bitrate);
  AacpAudioContext *s = malloc(sizeof(AacpAudioContext));
  if (s == NULL) 
    caml_failwith("malloc");  
  
  s->coreWriteOffset = 0;
  s->envReadOffset = 0;
  s->writeOffset=INPUT_DELAY*MAX_CHANNELS;

  s->useParametricStereo = 0;
  s->numAncDataBytes=0;
  s->adtsOffset=ADTS_HEADER_SIZE;


  /* number of channels */
  if (channels < 1 || channels > 2) {
    free(s);
    caml_raise_constant(*caml_named_value("aacplus_exn_no_sbr_settings"));
  }
    
  AacInitDefaultConfig(&s->config);
  s->nChannelsAAC = s->nChannelsSBR = channels;
  s->sampleRateAAC = sample_rate;
    
  /* ps or sbr? */
  if ( (channels == 2) && (bit_rate >= 16000) && (bit_rate < 44001) ) {
    s->useParametricStereo = 1;
    s->nChannelsAAC = 1;

    s->envReadOffset = (MAX_DS_FILTER_DELAY + INPUT_DELAY)*MAX_CHANNELS;
    s->coreWriteOffset = CORE_INPUT_OFFSET_PS;
    s->writeOffset = s->envReadOffset;
  } else {
    /* set up 2:1 downsampling */
    InitIIR21_Resampler(&(s->IIR21_reSampler[0]));
    if (channels == 2) InitIIR21_Resampler(&(s->IIR21_reSampler[1]));
      if (s->IIR21_reSampler[0].delay > MAX_DS_FILTER_DELAY) {
        free(s);
        caml_raise_constant(*caml_named_value("aacplus_exn_resampler_size"));
      }
      s->writeOffset += s->IIR21_reSampler[0].delay * channels; //MAX_CHANNELS;
  }
    
  s->config.bitRate = bit_rate;
  s->config.nChannelsIn=channels;
  s->config.nChannelsOut=s->nChannelsAAC;
  s->config.bandWidth=0;
    
  /* set up SBR configuration	*/
  if(!IsSbrSettingAvail (bit_rate, s->nChannelsAAC, s->sampleRateAAC, &s->sampleRateAAC)) {
    free(s);
    caml_raise_constant(*caml_named_value("aacplus_exn_no_sbr_settings"));
  }

  InitializeSbrDefaults (&s->sbrConfig);
  s->sbrConfig.usePs = s->useParametricStereo;
    
  AdjustSbrSettings(&s->sbrConfig, bit_rate, s->nChannelsAAC, s->sampleRateAAC, AACENC_TRANS_FAC, 24000);
  EnvOpen(&s->hEnvEnc, s->inputBuffer + s->coreWriteOffset, &s->sbrConfig, &s->config.bandWidth);
    
  /* set up AAC encoder, now that samling rate is known */
  s->config.sampleRate = s->sampleRateAAC;
  if (AacEncOpen(&s->aacEnc, s->config) != 0){
    AacEncClose(s->aacEnc);
    caml_raise_constant(*caml_named_value("aacplus_exn_encoder_init_failed"));
  }
   
  adts_hdr(s->adtsDataBytes, &s->config);

  ans = caml_alloc_custom(&aac_env_ops, sizeof(AacpAudioContext*), 1, 0);
  Aac_env_val(ans) = s;

  CAMLreturn(ans);
}

CAMLprim value ocaml_aacplus_encode_frame(value aac_env, value data)
{
  CAMLparam2(aac_env, data);
  CAMLlocal2(ans,datac);  
  
  AacpAudioContext *s = Aac_env_val(aac_env);

  int i, ch, outSamples, bytes_written;
  short *TimeDataPcm = (short *)String_val(data);

  for(i = 0; i < AACENC_BLOCKSIZE*2*s->nChannelsSBR; i++)
    s->inputBuffer[i+s->writeOffset] = TimeDataPcm[i];

  /* Register env as global root */
  caml_register_global_root(&aac_env);
  /* Got to blocking section */
  caml_enter_blocking_section();

  /* encode one SBR frame */
  EnvEncodeFrame( s->hEnvEnc, s->inputBuffer + s->envReadOffset, 
		  s->inputBuffer + s->coreWriteOffset, s->nChannelsSBR, 
		  &s->numAncDataBytes, s->ancDataBytes);
  
  /* 2:1 downsampling for AAC core */
  if (!s->useParametricStereo) {
    for( ch=0; ch < s->nChannelsSBR; ch++ )
      IIR21_Downsample( &(s->IIR21_reSampler[ch] ), s->inputBuffer + s->writeOffset+ch, 
                        AACENC_BLOCKSIZE * 2, s->nChannelsSBR, s->inputBuffer + ch, &outSamples, s->nChannelsSBR);
  }
    
  /* encode one AAC frame */
  AacEncEncode( s->aacEnc, s->inputBuffer, 
                s->useParametricStereo ? 1 : s->nChannelsSBR, /* stride (step) */ s->ancDataBytes, 
                &s->numAncDataBytes, (unsigned *) (s->frame + s->adtsOffset), &bytes_written);
    
  /* write ADTS header (if needed) */
  if(s->adtsOffset){
    memcpy(s->frame, s->adtsDataBytes, s->adtsOffset);
    adts_hdr_up(s->frame, bytes_written);
    bytes_written += s->adtsOffset;
  }
 
  if (s->useParametricStereo){
    memcpy( s->inputBuffer,s->inputBuffer+AACENC_BLOCKSIZE,CORE_INPUT_OFFSET_PS*sizeof(float));
  } else {
    memmove( s->inputBuffer,s->inputBuffer+AACENC_BLOCKSIZE*2*s->nChannelsSBR,s->writeOffset*sizeof(float));
  }

  /* Remove env from global root */
  caml_remove_global_root(&aac_env);
  /* Leave blocking section */
  caml_leave_blocking_section();

  ans = caml_alloc_string(bytes_written);
  memcpy(String_val(ans), s->frame, bytes_written);

  CAMLreturn(ans);
}
