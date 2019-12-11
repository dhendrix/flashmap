#!/usr/bin/env python
#
# Copyright 2010, Google Inc.
# All rights reserved.
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions are
# met:
#
#    * Redistributions of source code must retain the above copyright
# notice, this list of conditions and the following disclaimer.
#    * Redistributions in binary form must reproduce the above
# copyright notice, this list of conditions and the following disclaimer
# in the documentation and/or other materials provided with the
# distribution.
#    * Neither the name of Google Inc. nor the names of its
# contributors may be used to endorse or promote products derived from
# this software without specific prior written permission.
#
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
# "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
# LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
# A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
# OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
# SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
# LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
# DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
# THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
# (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
# OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
#
# Alternatively, this software may be distributed under the terms of the
# GNU General Public License ("GPL") version 2 as published by the Free
# Software Foundation.

"""
This module provides basic encode and decode functionality to the flashrom
memory map (FMAP) structure.

Usage:
  (decode)
  obj = fmap_decode(blob)
  print obj

  (encode)
  blob = fmap_encode(obj)
  open('output.bin', 'w').write(blob)

  The object returned by fmap_decode is a dictionary with names defined in
  fmap.h. A special property 'FLAGS' is provided as a readable and read-only
  tuple of decoded area flags.
"""


import logging
import struct
import sys


# constants imported from lib/fmap.h
FMAP_SIGNATURE = '__FMAP__'
FMAP_VER_MAJOR = 1
FMAP_VER_MINOR_MIN = 0
FMAP_VER_MINOR_MAX = 1
FMAP_STRLEN = 32
FMAP_SEARCH_STRIDE = 4

FMAP_FLAGS = {
    'FMAP_AREA_STATIC': 1 << 0,
    'FMAP_AREA_COMPRESSED': 1 << 1,
}

FMAP_HEADER_NAMES = (
    'signature',
    'ver_major',
    'ver_minor',
    'base',
    'size',
    'name',
    'nareas',
)

FMAP_AREA_NAMES = (
    'offset',
    'size',
    'name',
    'flags',
)


# format string
FMAP_HEADER_FORMAT = '<8sBBQI%dsH' % (FMAP_STRLEN)
FMAP_AREA_FORMAT = '<II%dsH' % (FMAP_STRLEN)


def _fmap_decode_header(blob, offset):
  """ (internal) Decodes a FMAP header from blob by offset"""
  header = {}
  for (name, value) in zip(FMAP_HEADER_NAMES,
                           struct.unpack_from(FMAP_HEADER_FORMAT,
                                              blob,
                                              offset)):
    header[name] = value

  if header['signature'] != FMAP_SIGNATURE:
    raise struct.error('Invalid signature')
  if (header['ver_major'] != FMAP_VER_MAJOR or
      header['ver_minor'] < FMAP_VER_MINOR_MIN or
      header['ver_minor'] > FMAP_VER_MINOR_MAX):
    raise struct.error('Incompatible version')

  # convert null-terminated names
  header['name'] = header['name'].strip(chr(0))
  return (header, struct.calcsize(FMAP_HEADER_FORMAT))


def _fmap_decode_area(blob, offset):
  """ (internal) Decodes a FMAP area record from blob by offset """
  area = {}
  for (name, value) in zip(FMAP_AREA_NAMES,
                           struct.unpack_from(FMAP_AREA_FORMAT, blob, offset)):
    area[name] = value
  # convert null-terminated names
  area['name'] = area['name'].strip(chr(0))
  # add a (readonly) readable FLAGS
  area['FLAGS'] = _fmap_decode_area_flags(area['flags'])
  return (area, struct.calcsize(FMAP_AREA_FORMAT))


def _fmap_decode_area_flags(area_flags):
  """ (internal) Decodes a FMAP flags property """
  return tuple([name for name in FMAP_FLAGS if area_flags & FMAP_FLAGS[name]])


def _fmap_check_name(fmap, name):
  """Checks if the FMAP structure has correct name.

  Args:
    fmap: A decoded FMAP structure.
    name: A string to specify expected FMAP name.

  Raises:
    struct.error if the name does not match.
  """
  if fmap['name'] != name:
    raise struct.error('Incorrect FMAP (found: "%s", expected: "%s")' %
                       (fmap['name'], name))


def _fmap_search_header(blob, fmap_name=None):
  """Searches FMAP headers in given blob.

  Uses same logic from vboot_reference/host/lib/fmap.c.

  Args:
    blob: A string containing FMAP data.
    fmap_name: A string to specify target FMAP name.

  Returns:
    A tuple of (fmap, size, offset).
  """
  lim = len(blob) - struct.calcsize(FMAP_HEADER_FORMAT)
  align = FMAP_SEARCH_STRIDE

  # Search large alignments before small ones to find "right" FMAP.
  while align <= lim:
    align *= 2

  while align >= FMAP_SEARCH_STRIDE:
    for offset in xrange(align, lim + 1, align * 2):
      if not blob.startswith(FMAP_SIGNATURE, offset):
        continue
      try:
        (fmap, size) = _fmap_decode_header(blob, offset)
        if fmap_name is not None:
          _fmap_check_name(fmap, fmap_name)
        return (fmap, size, offset)
      except struct.error as e:
        # Search for next FMAP candidate.
        logging.debug('Continue searching FMAP due to exception %r', e)
        pass
    align /= 2
  raise struct.error('No valid FMAP signatures.')


def fmap_decode(blob, offset=None, fmap_name=None):
  """ Decodes a blob to FMAP dictionary object.

  Arguments:
    blob: a binary data containing FMAP structure.
    offset: starting offset of FMAP. When omitted, fmap_decode will search in
            the blob.
  """
  fmap = {}

  if offset is None:
    (fmap, size, offset) = _fmap_search_header(blob, fmap_name)
  else:
    (fmap, size) = _fmap_decode_header(blob, offset)
    if fmap_name is not None:
      _fmap_check_name(fmap, fmap_name)
  fmap['areas'] = []
  offset = offset + size
  for _ in range(fmap['nareas']):
    (area, size) = _fmap_decode_area(blob, offset)
    offset = offset + size
    fmap['areas'].append(area)
  return fmap


def _fmap_encode_header(obj):
  """ (internal) Encodes a FMAP header """
  values = [obj[name] for name in FMAP_HEADER_NAMES]
  return struct.pack(FMAP_HEADER_FORMAT, *values)


def _fmap_encode_area(obj):
  """ (internal) Encodes a FMAP area entry """
  values = [obj[name] for name in FMAP_AREA_NAMES]
  return struct.pack(FMAP_AREA_FORMAT, *values)


def fmap_encode(obj):
  """ Encodes a FMAP dictionary object to blob.

  Arguments
    obj: a FMAP dictionary object.
  """
  # fix up values
  obj['nareas'] = len(obj['areas'])
  # TODO(hungte) re-assign signature / version?
  blob = _fmap_encode_header(obj)
  for area in obj['areas']:
    blob = blob + _fmap_encode_area(area)
  return blob


def main():
  """Decode FMAP from supplied file and print."""
  if len(sys.argv) < 2:
    print 'Usage: fmap.py <file>'
    sys.exit(1)

  filename = sys.argv[1]
  print 'Decoding FMAP from: %s' % filename
  blob = open(filename).read()
  obj = fmap_decode(blob)
  print obj


if __name__ == '__main__':
  main()
