#ifndef _CREATEUIWIDGETUI_H_
#define _CREATEUIWIDGETUI_H_

#include <QWidget>
#include <AudioUnit/AudioUnit.h>

QWidget *createUIWidget(CFURLRef bundleLocation, CFStringRef viewClass, AudioUnit unit, QWidget *parent);

#endif /* _CREATEUIWIDGET_H_ */
