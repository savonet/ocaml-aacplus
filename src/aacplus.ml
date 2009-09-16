(*
 * Copyright 2003-2009 Savonet team
 *
 * This file is part of Ocaml-aacplus.
 *
 * Ocaml-aacplus is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * Ocaml-aacplus is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Ocaml-aacplus; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *)

 (** OCaml bindings for the libaacplus. *)

type t

exception IIR21_reSampler_size_too_big
exception No_sbr_settings
exception Create_failed
exception Invalid_size

let () =
  Callback.register_exception "aacplus_exn_resampler_size" IIR21_reSampler_size_too_big;
  Callback.register_exception "aacplus_exn_no_sbr_settings" No_sbr_settings;
  Callback.register_exception "aacplus_exn_encoder_init_failed" Create_failed

external init : unit -> unit = "ocaml_aacplus_init"

external destroy : unit -> unit = "ocaml_aacplus_destroy"

external create : int -> int -> int -> t = "ocaml_aacplus_init_enc"

let create ~channels ~samplerate ~bitrate () = 
  create channels samplerate bitrate

external data_length : t -> int = "ocaml_aacplus_block_size"

external encode : t -> string -> string = "ocaml_aacplus_encode_frame"

let encode enc data = 
  if String.length data <> data_length enc then
    raise Invalid_size;
  encode enc data
