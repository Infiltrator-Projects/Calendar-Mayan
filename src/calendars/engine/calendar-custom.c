// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 1993-2026 Shannon Smith

/* Maya Long Count chronology: a single provider with fixed 20-day
 * uinals and 360-day tuns. The public ABI remains unchanged. */

#include "calendar-custom.h"
#include "calendar-ancient.h"
#include "calendar-bahai.h"
#include "calendar-helpers.h"
#include "calendar-historical.h"
#include "calendar-perpetual.h"
#include "calendar-reform.h"
#include "julian-day.h"

#include <glib/gi18n-lib.h>

typedef CalendarCalendarMode CalendarMode;
typedef CalendarCalendarFields CalendarFields;

typedef void (*FieldsFromJdnFunc)(gint64 jdn, CalendarFields *fields);
typedef gint64 (*FieldsToJdnFunc)(const CalendarFields *fields);
typedef gchar *(*FormatFunc)(const CalendarFields *, CalendarDatePart);

typedef struct
{
    FieldsFromJdnFunc fields_from_jdn;
    FieldsToJdnFunc fields_to_jdn;
    FormatFunc format;
    gint fixed_period_days;
    gint fixed_year_days;
    gint period_origin_day;
} CustomCalendarProvider;

/* One chronology means one descriptor, rather than a sparse mode registry. */
static const CustomCalendarProvider mayan_provider = {
    calendar_mayan_fields_from_jdn,
    calendar_mayan_fields_to_jdn,
    calendar_mayan_format,
    20, 360, 0
};

static const CustomCalendarProvider *
custom_provider(CalendarMode mode)
{
    return mode == CALENDAR_CALENDAR_MODE_MAYAN ? &mayan_provider : NULL;
}

void
calendar_custom_fields_from_jdn(CalendarMode mode,
                                     gint64 jdn,
                                     CalendarFields *fields)
{
    const CustomCalendarProvider *provider = custom_provider(mode);

    g_return_if_fail(fields != NULL);
    *fields = (CalendarFields){0};

    if (provider != NULL)
        provider->fields_from_jdn(jdn, fields);
}

gchar *
calendar_custom_format(CalendarMode mode,
                            const CalendarFields *fields,
                            CalendarDatePart part)
{
    const CustomCalendarProvider *provider = custom_provider(mode);

    if (provider == NULL || fields == NULL || provider->format == NULL)
        return g_strdup("");

    return provider->format(fields, part);
}

gint64
calendar_custom_month_start(CalendarMode mode,
                                 gint64 jdn)
{
    const CustomCalendarProvider *provider = custom_provider(mode);
    CalendarFields fields;

    if (provider == NULL)
        return CALENDAR_UNIX_EPOCH_JDN;

    provider->fields_from_jdn(jdn, &fields);
    fields.day = provider->period_origin_day;
    return provider->fields_to_jdn(&fields);
}

gint64
calendar_custom_add_months(CalendarMode mode, gint64 jdn, gint amount)
{
    const CustomCalendarProvider *provider = custom_provider(mode);
    if (provider == NULL)
        return jdn;
    return calendar_i64_add_saturating(
        jdn, calendar_i64_multiply_saturating(amount, provider->fixed_period_days));
}

gint64
calendar_custom_add_years(CalendarMode mode, gint64 jdn, gint amount)
{
    const CustomCalendarProvider *provider = custom_provider(mode);
    if (provider == NULL)
        return jdn;
    return calendar_i64_add_saturating(
        jdn, calendar_i64_multiply_saturating(amount, provider->fixed_year_days));
}
