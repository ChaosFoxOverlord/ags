//=============================================================================
//
// Adventure Game Studio (AGS)
//
// Copyright (C) 1999-2011 Chris Jones and 2011-20xx others
// The full list of copyright holders can be found in the Copyright.txt
// file, which is part of this source code distribution.
//
// The AGS source code is provided under the Artistic License 2.0.
// A copy of this license can be found in the file License.txt and at
// http://www.opensource.org/licenses/artistic-license-2.0.php
//
//=============================================================================
#include "ac/dynobj/scriptfile.h"
#include "ac/global_file.h"

// CHECKME: actually NULLs here will be equal to kFile_Open & kFile_Read
const Common::FileOpenMode sc_File::fopenModes[] = 
    {Common::kFile_Open/*CHECKME, was undefined*/, Common::kFile_Open, Common::kFile_CreateAlways, Common::kFile_Create};
const Common::FileWorkMode sc_File::fworkModes[] = 
    {Common::kFile_Read/*CHECKME, was undefined*/, Common::kFile_Read, Common::kFile_Write, Common::kFile_Write};

int sc_File::Dispose(void* /*address*/, bool /*force*/) {
    Close();
    delete this;
    return 1;
}

const char *sc_File::GetType() {
    return "File";
}

int sc_File::Serialize(void* /*address*/, uint8_t* /*buffer*/, int /*bufsize*/) {
    // we cannot serialize an open file, so it will get closed
    return 0;
}

int sc_File::OpenFile(const char *filename, int mode) {
  handle = FileOpen(filename, fopenModes[mode], fworkModes[mode]);
  if (handle <= 0)
      return 0;
  return 1;
}

void sc_File::Close() {
  if (handle > 0) {
    FileClose(handle);
    handle = 0;
  }
}

sc_File::sc_File() {
    handle = 0;
}
