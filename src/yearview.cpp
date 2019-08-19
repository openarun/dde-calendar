
/*
 * Copyright (C) 2015 ~ 2018 Deepin Technology Co., Ltd.
 *
 * Author:     kirigaya <kirigaya@mkacg.com>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "calendardbus.h"

#include <QGridLayout>
#include <QLabel>
#include <QPainter>
#include <QEvent>
#include <QDebug>
#include <QMessageBox>
#include <QTime>
#include <QQueue>
#include "yearview.h"
#include <QSpacerItem>
CYearView::CYearView(QWidget *parent) : QWidget(parent)
{
    m_DBusInter = new CalendarDBus("com.deepin.api.LunarCalendar",
                                   "/com/deepin/api/LunarCalendar",
                                   QDBusConnection::sessionBus(), this);
    queue = nullptr;
    lunarCache = nullptr;
    emptyCaLunarDayInfo = nullptr;

    if (!queue)
        queue = new QQueue<int>;
    if (!lunarCache)
        lunarCache = new QMap<QDate, CaLunarDayInfo>;
    if (!emptyCaLunarDayInfo)
        emptyCaLunarDayInfo = new CaLunarDayInfo;

    m_dayNumFont.setFamily("Helvetica");
    m_dayNumFont.setPixelSize(12);
    m_dayNumFont.setWeight(QFont::Light);

    //setStyleSheet("QWidget { background: rgba(0, 0, 0, 0) }");

    //add separator line
    m_currentMouth = new QLabel();
    m_currentMouth->setFixedHeight(DDEYearCalendar::Y_MLableHeight);
    //m_currentMouth->setStyleSheet("border: 1px solid rgba(0, 0, 0, 0.05);");

    QFont t_labelF;
    t_labelF.setFamily("Helvetica");
    t_labelF.setPixelSize(16);
    QPalette t_pa;
    t_pa.setColor(QPalette::WindowText,QColor("#CF0059 "));
    m_currentMouth->setFont(t_labelF);
    m_currentMouth->setStyleSheet("color:#CF0059;");
    QHBoxLayout* separatorLineLayout = new QHBoxLayout;
    separatorLineLayout->setMargin(0);
    separatorLineLayout->setSpacing(0);
    separatorLineLayout->setContentsMargins(10, 0, 0, 0);
    separatorLineLayout->addWidget(m_currentMouth);
    QSpacerItem *t_spaceitem = new QSpacerItem(30,DDEYearCalendar::Y_MLableHeight,QSizePolicy::Expanding,QSizePolicy::Fixed);
    separatorLineLayout->addSpacerItem(t_spaceitem);

    // cells grid
    QGridLayout *gridLayout = new QGridLayout;
    gridLayout->setMargin(0);
    gridLayout->setSpacing(0);
    for (int r = 0; r != 6; ++r) {
        for (int c = 0; c != 7; ++c) {
            QWidget *cell = new QWidget;
            cell->setFixedSize(DDEYearCalendar::YCellWidth, DDEYearCalendar::YCellHeight);
            cell->installEventFilter(this);
            cell->setFocusPolicy(Qt::ClickFocus);
            gridLayout->addWidget(cell, r, c);
            m_cellList.append(cell);
        }
    }
    QVBoxLayout *hhLayout = new QVBoxLayout;
    hhLayout->addLayout(separatorLineLayout);
    hhLayout->addLayout(gridLayout);
    QWidget *gridWidget = new QWidget;
    gridWidget->setLayout(hhLayout);
    QVBoxLayout *mainLayout = new QVBoxLayout;
   // mainLayout->addWidget(m_weekIndicator, 0, Qt::AlignHCenter);
    mainLayout->addWidget(gridWidget, 0,  Qt::AlignHCenter);
    mainLayout->setMargin(0);
    mainLayout->setSpacing(0);

    setLayout(mainLayout);

    connect(this, &CYearView::dateSelected, this, &CYearView::handleCurrentDateChanged);
    setFixedSize(DDEYearCalendar::Y_MWindowWidth,DDEYearCalendar::Y_MWindowHeight);
}
void CYearView::handleCurrentDateChanged(const QDate date, const CaLunarDayInfo &detail) {
    return;
    Q_UNUSED(detail);


    if (date != m_currentDate) {
        setCurrentDate(date);
    }
}

void CYearView::setFirstWeekday(int weekday)
{
    m_firstWeekDay = weekday;
    updateDate();
}

int CYearView::getDateType(const QDate &date)
{
    const int currentIndex = getDateIndex(date);
    const CaLunarDayInfo info = getCaLunarDayInfo(currentIndex);

    const int dayOfWeek = date.dayOfWeek();
    bool weekends = dayOfWeek == 6 || dayOfWeek == 7;
    bool isCurrentMonth = m_currentDate.month() == date.month();
    bool isFestival = !info.mSolarFestival.isEmpty() || !info.mLunarFestival.isEmpty();

    int resultFlag = 0;
    if (!isCurrentMonth)
        resultFlag |= SO_YNotCurrentMonth;
    if (isFestival)
        resultFlag |= SO_YFestival;
    if (weekends)
        resultFlag |= SO_YWeekends;

    return resultFlag;
}

void CYearView::updateSelectState()
{
    m_selectedCell = -1;
    update();
}

void CYearView::setCurrentDate(const QDate date)
{
    qDebug() << "set current date " << date;

    if (date == m_currentDate) {
        return;
    }

    m_currentDate = date;
    m_currentMouth->setText(QString::number(date.month()) + tr("Mon"));
    getDateType(m_currentDate);
    updateDate();
}

void CYearView::setCellSelectable(bool selectable)
{
    if (selectable == m_cellSelectable)
        return;
    m_cellSelectable = selectable;
}

int CYearView::getDateIndex(const QDate &date) const
{
    for (int i = 0; i != 42; ++i)
        if (m_days[i] == date)
            return i;

    return 0;
}

bool CYearView::eventFilter(QObject *o, QEvent *e)
{
    QWidget *cell = qobject_cast<QWidget *>(o);

    if (cell && m_cellList.contains(cell)) {
        if (e->type() == QEvent::Paint) {
            paintCell(cell);
        } else if (e->type() == QEvent::MouseButtonPress) {
            cellClicked(cell);
        }
    }

    return false;
}

void CYearView::updateDate()
{
    const QDate firstDay(m_currentDate.year(), m_currentDate.month(), 1);
    const int day = (firstDay.dayOfWeek() + m_firstWeekDay) % 7;
    const int currentIndex = day + m_currentDate.day() - 1;

    if (currentIndex < 0) {
        return;
    }

    for (int i(0); i != 42; ++i) {
        m_days[i] = firstDay.addDays(i - day);
    }

    //setSelectedCell(currentIndex);
    update();
}
const QString CYearView::getCellDayNum(int pos)
{
    return QString::number(m_days[pos].day());
}

const QDate CYearView::getCellDate(int pos)
{
    return m_days[pos];
}

const QString CYearView::getLunar(int pos)
{
    CaLunarDayInfo info = getCaLunarDayInfo(pos);

    if (info.mLunarDayName == "初一") {
        info.mLunarDayName = info.mLunarMonthName;
    }

    if (info.mTerm.isEmpty())
        return info.mLunarDayName;

    return info.mTerm;
}

const CaLunarDayInfo CYearView::getCaLunarDayInfo(int pos)
{
    const QDate date = m_days[pos];

    if (lunarCache->contains(date)) {
        return lunarCache->value(date);
    }

    if (lunarCache->size() > 300)
        lunarCache->clear();

//    QTimer::singleShot(500, [this, pos] {getDbusData(pos);});
    queue->push_back(pos);

    QTimer::singleShot(300, this, SLOT(getDbusData()));

    return *emptyCaLunarDayInfo;
}

void CYearView::getDbusData()
{
    if (queue->isEmpty())
        return;

    const int pos = queue->head();
    queue->pop_front();
    const QDate date = m_days[pos];
    if (!date.isValid()) {
        return;
    }

    CaLunarDayInfo currentDayInfo;
    if (!lunarCache->contains(date)) {
        bool o1 = true;
        QDBusReply<CaLunarMonthInfo> reply = m_DBusInter->GetLunarMonthCalendar(date.year(), date.month(), false, o1);

        QDate cacheDate;
        cacheDate.setDate(date.year(), date.month(), 1);
        foreach(const CaLunarDayInfo & dayInfo, reply.value().mCaLunarDayInfo) {
            lunarCache->insert(cacheDate, dayInfo);
            if (date == m_currentDate) {
                currentDayInfo = dayInfo;
            }
            cacheDate = cacheDate.addDays(1);
        }
    } else {
        currentDayInfo = lunarCache->value(date);
    }
    m_cellList.at(pos)->update();
    // refresh   lunar info
    if (date == m_currentDate) {
        emit datecurrentDateChanged(date,getCaLunarDayInfo(getDateIndex(m_currentDate)));
    }
}

void CYearView::paintCell(QWidget *cell)
{
    const QRect rect((cell->width() - DDEYearCalendar::YHeaderItemWidth) /2,
                     (cell->height() - DDEYearCalendar::YHeaderItemHeight) /2,
                     DDEYearCalendar::YCellHighlightWidth,
                     DDEYearCalendar::YCellHighlightHeight);

    const int pos = m_cellList.indexOf(cell);
    const int type = getDateType(m_days[pos]);
    const bool isSelectedCell = pos == m_selectedCell;
    const bool isCurrentDay = getCellDate(pos) == QDate::currentDate();

    QPainter painter(cell);

//    painter.drawRoundedRect(cell->rect(), 4, 4);

    // draw selected cell background circle
    if (isSelectedCell)
    {
        QRect fillRect = rect;

        painter.setRenderHints(QPainter::HighQualityAntialiasing);
        painter.setBrush(QBrush(m_backgroundCircleColor));
        painter.setPen(Qt::NoPen);
        painter.drawRoundedRect(fillRect, 4, 4);
    }

    painter.setPen(Qt::SolidLine);

    const QString dayNum = getCellDayNum(pos);
    const QString dayLunar = getLunar(pos);

    // draw text of day
    if (isSelectedCell) {
        painter.setPen(m_selectedTextColor);
    } else if (isCurrentDay) {
        painter.setPen(m_currentDayTextColor);
    } else {
        const int tType = type & 0xff;
        if (tType & SO_YNotCurrentMonth)
            painter.setPen(m_notCurrentTextColor);
        else if (type == SO_YWeekends)
            painter.setPen(m_weekendsTextColor);
        else
            painter.setPen(m_defaultTextColor);
    }

//    painter.drawRect(rect);
    QRect test;
    painter.setFont(m_dayNumFont);

    painter.drawText(rect, Qt::AlignCenter, dayNum, &test);

    painter.end();
}

void CYearView::cellClicked(QWidget *cell)
{
    if (!m_cellSelectable)
        return;

    const int pos = m_cellList.indexOf(cell);
    if (pos == -1)
        return;

    setSelectedCell(pos);

    // my gift eggs
    static int gift = 0;
    if (m_days[pos] == QDate(1993, 7, 28))
        if (++gift == 10)
            QMessageBox::about(this, "LinuxDeepin", "by shibowen <sbw@sbw.so> :P");
}

void CYearView::setSelectedCell(int index)
{
    if (m_selectedCell == index)
        return;

    const int prevPos = m_selectedCell;
    m_selectedCell = index;

    updateDate();
    //m_cellList.at(prevPos)->update();
    //m_cellList.at(index)->update();
    emit singanleActiveW(this);
    emit dateSelected(m_days[index], getCaLunarDayInfo(index));
}
