#ifndef SEEKSLIDER_H
#define SEEKSLIDER_H

#include <QSlider>

class SeekSlider : public QSlider
{
    Q_OBJECT

public:
    explicit SeekSlider(Qt::Orientation orientation, QWidget *parent = nullptr);

protected:
    void mousePressEvent(QMouseEvent *event) override;
};

#endif // SEEKSLIDER_H
