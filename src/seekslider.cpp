#include "seekslider.h"

#include <QMouseEvent>
#include <QStyle>
#include <QStyleOptionSlider>

SeekSlider::SeekSlider(Qt::Orientation orientation, QWidget *parent)
    : QSlider(orientation, parent)
{
}

void SeekSlider::mousePressEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton && maximum() > minimum()) {
        QStyleOptionSlider option;
        initStyleOption(&option);
        const QRect groove = style()->subControlRect(QStyle::CC_Slider, &option, QStyle::SC_SliderGroove, this);
        const QRect handle = style()->subControlRect(QStyle::CC_Slider, &option, QStyle::SC_SliderHandle, this);

        if (handle.contains(event->position().toPoint())) {
            QSlider::mousePressEvent(event);
            return;
        }

        const int sliderMin = groove.x();
        const int sliderMax = groove.right() - handle.width() + 1;
        const int clickPos = event->position().toPoint().x() - handle.width() / 2;
        const int value = QStyle::sliderValueFromPosition(minimum(), maximum(), clickPos - sliderMin, sliderMax - sliderMin);
        setValue(qBound(minimum(), value, maximum()));
        emit sliderMoved(this->value());
        emit sliderReleased();
        event->accept();
        return;
    }

    QSlider::mousePressEvent(event);
}
