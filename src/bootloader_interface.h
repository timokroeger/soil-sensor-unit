// Copyright (c) 2018 Timo Kröger <timokroeger93+code@gmail.com>

#ifndef BOOTLOADER_INTERFACE_H_
#define BOOTLOADER_INTERFACE_H_

#include <cstdint>
#include <cstdlib>

class BootloaderInterface {
 public:
  // Erases and prepares flash memory for writing a new image.
  // Returns true when successfull, false otherwise.
  virtual bool PrepareUpdate() = 0;

  // Writes parts of the update image data.
  // There may be restrictions on the length. See the implemenation for details.
  // Returns true when successfull, false otherwise.
  virtual bool WriteImageData(size_t offset, uint8_t* data, size_t length) = 0;

  // Mark update image as ready so that the bootloader copies and uses it after
  // the next reset.
  virtual bool SetUpdatePending() = 0;

  // Mark the current image as OK so that it permanently.
  // If a reset occures without the image being marked as ok the bootloader
  // reverts to the previous image.
  virtual bool SetUpdateConfirmed() = 0;
};

#endif  // BOOTLOADER_INTERFACE_H_
