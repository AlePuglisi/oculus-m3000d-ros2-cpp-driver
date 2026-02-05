/****************************************************************************
 **
 ** Copyright (C) Qxt Foundation. Some rights reserved.
 **
 ** This file is part of the QxtGui module of the Qxt library.
 **
 ** This library is free software; you can redistribute it and/or modify it
 ** under the terms of the Common Public License, version 1.0, as published
 ** by IBM, and/or under the terms of the GNU Lesser General Public License,
 ** version 2.1, as published by the Free Software Foundation.
 **
 ** This file is provided "AS IS", without WARRANTIES OR CONDITIONS OF ANY
 ** KIND, EITHER EXPRESS OR IMPLIED INCLUDING, WITHOUT LIMITATION, ANY
 ** WARRANTIES OR CONDITIONS OF TITLE, NON-INFRINGEMENT, MERCHANTABILITY OR
 ** FITNESS FOR A PARTICULAR PURPOSE.
 **
 ** You should have received a copy of the CPL and the LGPL along with this
 ** file. See the LICENSE file and the cpl1.0.txt/lgpl-2.1.txt files
 ** included with the source distribution for more information.
 ** If you did not receive a copy of the licenses, contact the Qxt Foundation.
 **
 ** <http://libqxt.org>  <foundation@libqxt.org>
 **
 ****************************************************************************/
#ifndef RangeSlider_P_H
#define RangeSlider_P_H

#include <QStyle>
#include <QObject>
#include "RangeSlider.h"


QT_FORWARD_DECLARE_CLASS(QStylePainter)
QT_FORWARD_DECLARE_CLASS(QStyleOptionSlider)

class RangeSliderPrivate : public QObject, public QxtPrivate<RangeSlider>
{
    Q_OBJECT

public:


		//QXT_DECLARE_PUBLIC(RangeSlider)

		friend class RangeSlider;

    RangeSliderPrivate();
    void initStyleOption(QStyleOptionSlider* option, RangeSlider::SpanHandle handle = RangeSlider::UpperHandle) const;
    int pick(const QPoint& pt) const
    {
        return qxt_p().orientation() == Qt::Horizontal ? pt.x() : pt.y();
    }
    int pixelPosToRangeValue(int pos) const;
    void handleMousePress(const QPoint& pos, QStyle::SubControl& control, int value, RangeSlider::SpanHandle handle);
    void drawHandle(QStylePainter* painter, RangeSlider::SpanHandle handle) const;
	void drawSomething(QStylePainter* painter, RangeSlider::SpanHandle handle) const;
	void setupPainter(QPainter* painter, Qt::Orientation orientation, qreal x1, qreal y1, qreal x2, qreal y2) const;
    void drawSpan(QStylePainter* painter, const QRect& rect) const;
    void triggerAction(QAbstractSlider::SliderAction action, bool main);
    void swapControls();

	//void drawMarkers(QStylePainter* painter, QStyleOptionSlider* option);

    int lower;
    int upper;
    int lowerPos;
    int upperPos;
		int current;
		int currentPos;
    int offset;
    int position;
    RangeSlider::SpanHandle lastPressed;
    RangeSlider::SpanHandle mainControl;
    QStyle::SubControl lowerPressed;
    QStyle::SubControl upperPressed;
	QStyle::SubControl currentPressed;
    RangeSlider::HandleMovementMode movement;
    bool firstMovement;
    bool blockTracking;


public Q_SLOTS:
    void updateRange(int min, int max);
    void movePressedHandle();
};

#endif // RangeSlider_P_H
