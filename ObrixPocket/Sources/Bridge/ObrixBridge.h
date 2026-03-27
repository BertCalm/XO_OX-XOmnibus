#ifndef ObrixBridge_h
#define ObrixBridge_h

#import <Foundation/Foundation.h>

/// Singleton bridge between Swift UI and JUCE-hosted ObrixEngine.
/// All methods are message-thread safe. Audio rendering happens on
/// JUCE's internal audio thread.
@interface ObrixBridge : NSObject

+ (instancetype)shared;

/// Audio lifecycle
- (void)startAudio;
- (void)stopAudio;
- (BOOL)isRunning;

/// Note input (from ReefGrid touch events)
- (void)noteOn:(int)note velocity:(float)velocity;
- (void)noteOff:(int)note;
- (void)allNotesOff;

/// Parameter control
- (void)setParameter:(NSString *)paramId value:(float)value;
- (float)getParameter:(NSString *)paramId;

/// Output gain (for ducking under other media)
- (void)setOutputGain:(float)gain;

/// Engine info
- (float)cpuLoad;
- (int)activeVoiceCount;
- (float)sampleRate;

@end

#endif
