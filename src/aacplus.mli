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

(** Initialize the encoding module. 
  * Must be called before anything happen. *)
val init : unit -> unit

(** Destroy anything create at [init].
  * Should be called when ending the encoding process. *)
val destroy : unit -> unit

(** Create a new encoder. Bitrate is in bits (e.g. 64000).
  *
  * Raises [No_sbr_settings] if no SBR settings match 
  * the requested settings.
  * 
  * Raises [IIR21_reSampler_size_too_big] is downsampling is
  * required and could not be enabled.
  *
  * Raises [Create_failed] if creation failed for another reason. *)
val create : channels:int -> samplerate:int -> bitrate:int -> unit -> t

(** Get encoder's data length. 
  *
  * Data string submited for encoding when using
  * [encode] should have be exactly
  * this length. *)
val data_length : t -> int

(** Encode data. 
  *
  * Input format: interleaved S16LE
  * data. 
  *
  * Raises [Invalid_size] if data string length is 
  * not exactly [data_length]. *)
val encode : t -> string -> string

