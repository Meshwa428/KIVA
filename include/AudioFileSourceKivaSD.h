#ifndef AUDIOFILESOURCEKIVASD_H
#define AUDIOFILESOURCEKIVASD_H

#include "AudioFileSource.h"
#include <FS.h> // Required for the File object

// A custom version of AudioFileSourceSD that uses our optimized SdCardManager
// to open files, ensuring high-speed reads for audio streaming.

class AudioFileSourceKivaSD : public AudioFileSource {
public:
  AudioFileSourceKivaSD();
  AudioFileSourceKivaSD(const char *filename);
  ~AudioFileSourceKivaSD() override;

  bool open(const char *filename) override;
  uint32_t read(void *data, uint32_t len) override;
  bool seek(int32_t pos, int dir) override;
  bool close() override;
  bool isOpen() override;
  uint32_t getSize() override;
  uint32_t getPos() override;

private:
  File f;
};

#endif // AUDIOFILESOURCEKIVASD_H