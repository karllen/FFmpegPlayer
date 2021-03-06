#pragma once

#include <stdint.h>
#include <memory>
#include <string>

#ifdef _WIN32
typedef std::wstring PathType;
#else
typedef std::string PathType;
#endif

struct FrameRenderingData
{
	uint8_t** image;
	int width; 
	int height;
};

struct IFrameListener
{
	virtual ~IFrameListener() {}
	virtual void updateFrame() = 0;
	virtual void drawFrame() = 0;
};

struct FrameDecoderListener
{
	virtual ~FrameDecoderListener() {}

	virtual void changedFramePosition(long long frame, long long total) {}
	virtual void decoderClosed() {}
	virtual void fileReleased() {}
	virtual void fileLoaded() {}
	virtual void processOpenning() {}
	virtual void volumeChanged(double volume) {}

	virtual void onEndOfStream() {}

	virtual void playingFinished() {}
};

struct IFrameDecoder
{
	enum FrameFormat {
		PIX_FMT_YUV420P,   ///< planar YUV 4:2:0, 12bpp, (1 Cr & Cb sample per 2x2 Y samples)
		PIX_FMT_YUYV422,   ///< packed YUV 4:2:2, 16bpp, Y0 Cb Y1 Cr
		PIX_FMT_RGB24,     ///< packed RGB 8:8:8, 24bpp, RGBRGB...
	};

	virtual ~IFrameDecoder() {}

	virtual void SetFrameFormat(FrameFormat format) = 0;

    virtual bool openFile(const PathType& file) = 0;
    virtual bool openUrl(const std::string& url) = 0;

	virtual void play(bool isPaused = false) = 0;
	virtual bool pauseResume() = 0;
	virtual void setVolume(double volume) = 0;

	virtual bool seekByPercent(double percent, int64_t totalDuration = -1) = 0;

	virtual void setFrameListener(IFrameListener* listener) = 0;
	virtual void setDecoderListener(FrameDecoderListener* listener) = 0;
	virtual bool getFrameRenderingData(FrameRenderingData* data) = 0;
	virtual void finishedDisplayingFrame() = 0;

	virtual void close() = 0;

	virtual bool isPlaying() const = 0;
	virtual bool isPaused() const = 0;
	virtual double volume() const = 0;
	virtual double getDurationSecs(int64_t duration) const = 0;
};

struct IAudioPlayer;

std::unique_ptr<IFrameDecoder> GetFrameDecoder(std::unique_ptr<IAudioPlayer> audioPlayer);
