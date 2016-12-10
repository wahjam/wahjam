#import <AudioUnit/AUCocoaUIView.h>
#include <QMacCocoaViewContainer>

QWidget *createUIWidget(CFURLRef bundleLocation,
                        CFStringRef viewClass,
                        AudioUnit unit,
                        QWidget *parent)
{
  NSBundle *bundle = [NSBundle bundleWithPath:[(NSURL *)bundleLocation path]];
  if (bundle == nil) {
    return NULL;
  }

  Class factoryClass = [bundle classNamed:(NSString *)viewClass];
  if (factoryClass == nil) {
    return NULL;
  }
  if (![factoryClass conformsToProtocol: @protocol(AUCocoaUIBase)]) {
    return NULL;
  }

  id factory = [[[factoryClass alloc] init] autorelease];
  if (factory == nil) {
    return NULL;
  }

  NSView *view = [factory uiViewForAudioUnit:unit withSize:NSZeroSize];
  return new QMacCocoaViewContainer(view, parent);
}
