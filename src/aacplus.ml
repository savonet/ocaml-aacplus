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

type priv

type t = int * int * int * priv

exception Invalid_data
exception Invalid_config

let () = Callback.register_exception "aacplus_exn_encoder_invalid_config" Invalid_config

external create : int -> int -> int -> t  = "ocaml_aacplus_init_enc"

let create ~channels ~samplerate ~bitrate () = 
  create channels samplerate bitrate

let frame_size (chans,x,_,_) = x / chans

external encode : priv -> float array array -> int -> string = "ocaml_aacplus_encode_frame"

let encode ((chans,_,out_len,enc) as h) data =
  if Array.length data <> chans then
    raise Invalid_data;
  Array.iter 
    (fun x -> if Array.length x <> frame_size h then raise Invalid_data)
    data ;
  encode enc data out_len

