#include "AudioFileSourceKivaSD.h"
#include "SdCardManager.h" // The key include for our optimized manager

AudioFileSourceKivaSD::AudioFileSourceKivaSD()
{
}

AudioFileSourceKivaSD::AudioFileSourceKivaSD(const char *filename)
{
  open(filename);
}

bool AudioFileSourceKivaSD::open(const char *filename)
{
  // MODIFICATION: Use our optimized file opener instead of the global SD.open()
  f = SdCardManager::getInstance().openFileUncached(filename, FILE_READ);
  return f;
}

AudioFileSourceKivaSD::~AudioFileSourceKivaSD()
{
  if (f) f.close();
}

uint32_t AudioFileSourceKivaSD::read(void *data, uint32_t len)
{
  if (!f) return 0;
  return f.read(reinterpret_cast<uint8_t*>(data), len);
}

bool AudioFileSourceKivaSD::seek(int32_t pos, int dir)
{
  if (!f) return false;
  if (dir==SEEK_SET) return f.seek(pos);
  else if (dir==SEEK_CUR) return f.seek(f.position() + pos);
  else if (dir==SEEK_END) return f.seek(f.size() + pos);
  return false;
}

bool AudioFileSourceKivaSD::close()
{
  if (f) {
    f.close();
    return true;
  }
  return false;
}

bool AudioFileSourceKivaSD::isOpen()
{
  return f ? true : false;
}

uint32_t AudioFileSourceKivaSD::getSize()
{
  if (!f) return 0;
  return f.size();
}

uint32_t AudioFileSourceKivaSD::getPos()
{
  if (!f) return 0;
  return f.position();
}