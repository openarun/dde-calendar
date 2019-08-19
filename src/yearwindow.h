/*
 * Copyright (C) 2017 ~ 2018 Deepin Technology Co., Ltd.
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
#ifndef YEARWINDOW_H
#define YEARWINDOW_H

#include <QWidget>
#include <DMainWindow>
#include <QDate>
#include <QLabel>
#include <dimagebutton.h>
#include <darrowbutton.h>
#include <dbasebutton.h>
#include <dlabel.h>
DWIDGET_USE_NAMESPACE

class CYearView;
class CaLunarDayInfo;
class CYearWindow: public QMainWindow
{
    Q_OBJECT
public:
    CYearWindow(QWidget *parent = 0);
    ~CYearWindow();
    void setDate(QDate date);
    void initUI();
    void initConnection();
signals:
    void dateSelected(const QDate date, const CaLunarDayInfo &detail) const;
private slots:
    void slotActiveW(CYearView * w);
    void slotprev();
    void slotnext();
    void slottoday();
    void slotdateSelected(const QDate date, const CaLunarDayInfo &detail) const;
    void slotdatecurrentDateChanged(const QDate date, const CaLunarDayInfo &detail) const;
private:
    QList<CYearView *> m_monthViewList;
    QFrame * m_contentBackground = nullptr;
    DArrowButton*      m_prevButton = nullptr;
    DArrowButton*      m_nextButton = nullptr;
    DBaseButton *      m_today = nullptr;
    QDate              m_currentdate;
    DLabel*            m_YearLabel;
    DLabel*            m_YearLunarLabel;
    CYearView *        m_activeview = nullptr;
};

#endif // YEARWINDOW_H
