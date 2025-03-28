/*
 * Copyright (C) 2020 Georg Zotti
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Suite 500, Boston, MA  02110-1335, USA.
 */

#ifndef OLDHINDULUNISOLARCALENDAR_HPP
#define OLDHINDULUNISOLARCALENDAR_HPP

#include "OldHinduSolarCalendar.hpp"

//! @class OldHinduLuniSolarCalendar
//! @brief Functions for the Old Hindu Lunisolar calendars described in CC:UE chapter 10.
//! @author Georg Zotti
//! @ingroup calendars
//! The old Hindu Lunisolar calendar as given in CC.UE describes the South Indian version where months begin at New Moon (amanta scheme).
//! The name of a lunar month depends on the solar month that begins during that lunar month.
//! A month is leap and takes the following month’s name when no solar month begins within it.
//! The calendar repeats after 180.000 years.

class OldHinduLuniSolarCalendar : public OldHinduSolarCalendar
{
	Q_OBJECT

public:
	OldHinduLuniSolarCalendar(double jd);

	~OldHinduLuniSolarCalendar() override {}

public slots:
	void retranslate() override;

	//! Set a calendar date from the Julian day number
	void setJD(double JD) override;

	//! set date from a vector of calendar date elements sorted from the largest to the smallest.
	//! Year-Month[1...12]-leap[0|1]-Day[1...30]
	void setDate(const QVector<int> &parts) override;

	//! get a stringlist of calendar date elements sorted from the largest to the smallest.
	//! {Year, Month, MonthName, leap[0|1], Day, DayName}
	QStringList getDateStrings() const override;

	//! get a formatted complete string for a date
	QString getFormattedDateString() const override;

	//! compute RD date from an Old Hindu Lunisolar date
	//! parts={ year, month, leap, day}
	static int fixedFromOldHinduLunar(const QVector<int> &parts);
	//! return { year, month, leap, day}
	static QVector<int> oldHinduLunarFromFixed(int rd);

	//! called old-hindu-lunar-leap-year?() in the CC.UE book.
	static bool isLeap(int lYear);

protected:
	constexpr static const double aryaLunarMonth   = 1577917500./53433336.0;
	constexpr static const double aryaLunarDay     = aryaLunarMonth/30.0;

	static QMap<int, QString> monthNames;
};

#endif
