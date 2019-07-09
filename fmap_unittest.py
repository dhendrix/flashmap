#!/usr/bin/python
#
# Copyright 2017 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.
"""Unit test for fmap module."""

import struct
import unittest

import fmap

# Expected decoded fmap structure from bin/example.bin
_EXAMPLE_BIN_FMAP = {
    'ver_major': 1,
    'ver_minor': 0,
    'name': 'example',
    'nareas': 4,
    'base': 0,
    'signature': '__FMAP__',
    'areas': [{
        'FLAGS': ('FMAP_AREA_STATIC',),
        'size': 128,
        'flags': 1,
        'name': 'bootblock',
        'offset': 0
    }, {
        'FLAGS': ('FMAP_AREA_COMPRESSED', 'FMAP_AREA_STATIC'),
        'size': 128,
        'flags': 3,
        'name': 'normal',
        'offset': 128
    }, {
        'FLAGS': ('FMAP_AREA_COMPRESSED', 'FMAP_AREA_STATIC'),
        'size': 256,
        'flags': 3,
        'name': 'fallback',
        'offset': 256
    }, {
        'FLAGS': (),
        'size': 512,
        'flags': 0,
        'name': 'data',
        'offset': 512
    }],
    'size': 1024
}


class FmapTest(unittest.TestCase):
  """Unit test for fmap module."""

  def setUp(self):
    with open('bin/example.bin') as f:
      self.example_blob = f.read()

  def testDecode(self):
    decoded = fmap.fmap_decode(self.example_blob)
    self.assertEquals(_EXAMPLE_BIN_FMAP, decoded)

  def testDecodeWithOffset(self):
    decoded = fmap.fmap_decode(self.example_blob, 512)
    self.assertEquals(_EXAMPLE_BIN_FMAP, decoded)

  def testDecodeWithName(self):
    decoded = fmap.fmap_decode(self.example_blob, fmap_name='example')
    self.assertEquals(_EXAMPLE_BIN_FMAP, decoded)
    decoded = fmap.fmap_decode(self.example_blob, 512, 'example')
    self.assertEquals(_EXAMPLE_BIN_FMAP, decoded)

  def testDecodeWithWrongName(self):
    with self.assertRaises(struct.error):
      decoded = fmap.fmap_decode(self.example_blob, fmap_name='banana')
    with self.assertRaises(struct.error):
      decoded = fmap.fmap_decode(self.example_blob, 512, 'banana')

  def testDecodeWithOffset(self):
    decoded = fmap.fmap_decode(self.example_blob, 512)
    self.assertEquals(_EXAMPLE_BIN_FMAP, decoded)

  def testDecodeWithWrongOffset(self):
    with self.assertRaises(struct.error):
      fmap.fmap_decode(self.example_blob, 42)

  def testEncode(self):
    encoded = fmap.fmap_encode(_EXAMPLE_BIN_FMAP)
    # example.bin contains other binary data besides the fmap
    self.assertIn(encoded, self.example_blob)


if __name__ == '__main__':
  unittest.main()
