#ifndef ObrixBridge_h
#define ObrixBridge_h

#import <Foundation/Foundation.h>

/// Singleton bridge between Swift UI and JUCE-hosted ObrixEngine.
/// All methods are message-thread safe. Audio rendering happens on
/// JUCE's internal audio thread.
@interface ObrixBridge : NSObject

+ (instancetype _Nullable)shared;

/// Audio lifecycle
- (void)startAudio;
- (void)stopAudio;
- (BOOL)isRunning;

/// Note input (from ReefGrid touch events)
- (void)noteOn:(int)note velocity:(float)velocity;
- (void)noteOff:(int)note;
- (void)allNotesOff;

/// Parameter control (queued — applied next audio callback)
- (void)setParameter:(NSString *)paramId value:(float)value;
- (float)getParameter:(NSString *)paramId;

/// Immediate parameter write — bypasses the queue, writes directly to the
/// atomic parameter value. Safe to call from any thread. Use this for
/// bulk configuration (e.g., applying a reef chain) where all params must
/// be set before the next noteOn.
- (void)setParameterImmediate:(NSString *)paramId value:(float)value;

/// Output gain (for ducking under other media)
- (void)setOutputGain:(float)gain;

/// Engine info
- (float)cpuLoad;
- (int)activeVoiceCount;
- (float)sampleRate;

// MARK: - Output tap (for AudioExporter live recording)

/// Start capturing processed audio output into the double-buffer tap.
/// Safe to call from any thread. No-op if tap is already active.
- (void)startOutputTap;

/// Stop the output tap and clear the ready buffer pointer.
/// Safe to call from any thread.
- (void)stopOutputTap;

/// Drain the latest captured interleaved PCM block into dest.
/// Returns the number of frames written (0 if no new buffer is available).
/// dest must be at least (maxFrames * 2) floats.
/// outChannels is set to the captured channel count (always ≤ 2).
/// Safe to call from the main thread only — never from the audio thread.
- (int)drainTapInto:(float * _Nonnull)dest
          maxFrames:(int)maxFrames
        outChannels:(int * _Nonnull)outChannels;

@end

#endif
