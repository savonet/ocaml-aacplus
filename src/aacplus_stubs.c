/*
 * OCaml bindings for libaacplus
 *
 * Copyright 2005-2010 Savonet team
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

#include <aacplus.h>
#include <string.h>

#define Aac_env_val(v) (*((aacplusEncHandle*)Data_custom_val(v)))

static void finalize_aac_env(value c)
{
  aacplusEncHandle h = Aac_env_val(c);
  aacplusEncClose(h);
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

CAMLprim value ocaml_aacplus_init_enc(value chans, value samplerate, value bitrate)
{
  CAMLparam0();
  CAMLlocal2(ret,ans);

  int channels = Int_val(chans);
  int sample_rate = Int_val(samplerate);
  int bit_rate = Int_val(bitrate);
  unsigned long inputsamples;
  unsigned long outbytes;
  aacplusEncHandle h = aacplusEncOpen(sample_rate,
                                      channels,
                                      &inputsamples,
                                      &outbytes);
  if (h == NULL) 
    caml_raise_out_of_memory();  

  aacplusEncConfiguration *cfg = aacplusEncGetCurrentConfiguration(h);
  cfg->bitRate = bit_rate;
  cfg->bandWidth = 0;
  cfg->inputFormat = AACPLUS_INPUT_FLOAT;
  cfg->outputFormat = 1;

  if (aacplusEncSetConfiguration(h,cfg) == 0) {
    aacplusEncClose(h);
    caml_raise_constant(*caml_named_value("aacplus_exn_encoder_invalid_config"));
  }

  ret = caml_alloc_tuple(4);
  Store_field(ret,0,chans);
  Store_field(ret,1,Val_int(inputsamples));
  Store_field(ret,2,Val_long(outbytes));

  ans = caml_alloc_custom(&aac_env_ops, sizeof(aacplusEncHandle), 1, 0);
  Aac_env_val(ans) = h;

  Store_field(ret,3,ans);

  CAMLreturn(ret);
}

static inline float clip(double s)
{
  if (s < -1)
  {
    return -1;
  }
  else if (s > 1)
  {
    return 1;
  }
  else
    return s;
}

CAMLprim value ocaml_aacplus_encode_frame(value aac_env, value data, value outlen)
{
  CAMLparam2(aac_env, data);
  CAMLlocal1(ans);  

  aacplusEncHandle h = Aac_env_val(aac_env);

  int channels = Wosize_val(data);
  if (channels < 1)
    caml_failwith("No data to encode!");
  int samples = Wosize_val(Field(data,0)) / Double_wosize;
  int len = samples * channels ;

  float *buf = malloc(len * sizeof(float)); // float samples
  if (buf == NULL)
    caml_raise_out_of_memory();

  int i, c;
  for (c = 0; c < channels; c++)
    for (i = 0; i < samples; i++)
      buf[i*channels + c] = clip(Double_field(Field(data,c),i));

  int out_len = Int_val(outlen);
  unsigned char *outbuf = malloc(out_len);
  if (outbuf == NULL)
  {
    free(buf);
    caml_raise_out_of_memory();
  }

  caml_enter_blocking_section();

  int bytes = aacplusEncEncode(h, (int32_t *)buf, len,
                               outbuf, out_len);

  caml_leave_blocking_section();
 
  free(buf);

  ans = caml_alloc_string(bytes);
  memcpy(String_val(ans),outbuf,bytes);
  free(outbuf);
 
  CAMLreturn(ans);

}
