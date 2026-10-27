// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 1993-2026 Shannon Smith

/*
 * Ancient-calendar engines.
 *
 * Mayan conversion uses the Goodman-Martínez-Thompson correlation constant
 * (JDN 584283), the conventional correlation adopted by the project's
 * reference literature (including Reingold/Dershowitz, Calendrical
 * Calculations). Alternative historical correlations exist, so Mayan absolute
 * dates are explicitly model-dependent rather than presented as uniquely
 * established chronology. The Long Count is treated as arithmetic over kin.
 * The native
 * representation stores tun / uinal / kin because those are the fields the UI
 * browses; conversion remains reversible even for dates before the epoch by
 * using floor division and a positive remainder.
 *
 * Roman dates are a formatting system over the proleptic Julian calendar, not
 * a second absolute chronology.  Roman day names count inclusively backwards
 * to Kalends, Nones or Ides, which is why the apparent distance includes both
 * the current day and the target marker.
 */

#include "calendar-ancient.h"

#include "julian-day.h"

#include <glib/gi18n-lib.h>

#define floor_divide calendar_floor_divide
#define positive_modulo calendar_positive_modulo

#define MAYAN_EPOCH_JDN ((gint64)584283)

enum
{
    MAYAN_KIN_PER_UINAL = 20,
    MAYAN_KIN_PER_TUN = 360,
    MAYAN_KIN_PER_HAAB = 365
};

static const gchar *const mayan_tzolkin_names[] = {
    "Imix", "Ik’", "Ak’bal", "K’an", "Chikchan",
    "Kimi", "Manik’", "Lamat", "Muluk", "Ok",
    "Chuwen", "Eb’", "B’en", "Ix", "Men",
    "Kib’", "Kab’an", "Etz’nab’", "Kawak", "Ajaw"
};

static const gchar *const mayan_haab_months[] = {
    "Pop", "Wo’", "Sip", "Sotz’", "Sek", "Xul",
    "Yaxk’in", "Mol", "Ch’en", "Yax", "Sak’", "Keh",
    "Mak", "K’ank’in", "Muwan’", "Pax", "K’ayab", "Kumk’u",
    "Wayeb’"
};


void
calendar_mayan_fields_from_jdn(
    gint64 jdn,
    CalendarCalendarFields *fields)
{
    gint64 total_kin;
    gint64 tun;
    gint kin_in_tun;

    g_return_if_fail(fields != NULL);
    if (!infiltratr_i64_add_checked(jdn, -MAYAN_EPOCH_JDN, &total_kin))
    {
        *fields = (CalendarCalendarFields){ 0 };
        return;
    }
    tun = floor_divide(total_kin, MAYAN_KIN_PER_TUN);
    kin_in_tun = (gint)positive_modulo(total_kin, MAYAN_KIN_PER_TUN);

    fields->year = tun;
    fields->month = kin_in_tun / MAYAN_KIN_PER_UINAL;
    fields->day = kin_in_tun % MAYAN_KIN_PER_UINAL;
    fields->auxiliary = 0;
    fields->special = FALSE;
}

gint64
calendar_mayan_fields_to_jdn(
    const CalendarCalendarFields *fields)
{
    g_return_val_if_fail(fields != NULL, MAYAN_EPOCH_JDN);

    gint64 result = MAYAN_EPOCH_JDN;

    result = calendar_i64_add_saturating(
        result,
        calendar_i64_multiply_saturating(
            fields->year, MAYAN_KIN_PER_TUN));
    result = calendar_i64_add_saturating(
        result,
        calendar_i64_multiply_saturating(
            fields->month, MAYAN_KIN_PER_UINAL));
    return calendar_i64_add_saturating(result, fields->day);
}


gchar *
calendar_mayan_format(
    const CalendarCalendarFields *fields,
    CalendarDatePart part)
{
    gint64 remaining;
    gint64 baktun;
    gint64 katun;
    gint64 tun;

    g_return_val_if_fail(fields != NULL, g_strdup(""));

    remaining = fields->year;
    baktun = floor_divide(remaining, 400);
    remaining = positive_modulo(remaining, 400);
    katun = remaining / 20;
    tun = remaining % 20;

    if (part == CALENDAR_DATE_PART_DAY)
        return g_strdup_printf("%d", fields->day);
    if (part == CALENDAR_DATE_PART_MONTH)
        return g_strdup_printf(_("Uinal %d"), fields->month);
    if (part == CALENDAR_DATE_PART_YEAR)
    {
        return g_strdup_printf("%" G_GINT64_FORMAT ".%" G_GINT64_FORMAT
                               ".%" G_GINT64_FORMAT,
                               baktun,
                               katun,
                               tun);
    }

    {
        gint64 total_kin = calendar_i64_multiply_saturating(
            fields->year, MAYAN_KIN_PER_TUN);

        total_kin = calendar_i64_add_saturating(
            total_kin,
            calendar_i64_multiply_saturating(
                fields->month, MAYAN_KIN_PER_UINAL));
        total_kin = calendar_i64_add_saturating(
            total_kin, fields->day);
        const gint tzolkin_number =
            (gint)positive_modulo(calendar_i64_add_saturating(total_kin, 3), 13) + 1;
        const gint tzolkin_name =
            (gint)positive_modulo(calendar_i64_add_saturating(total_kin, 19), 20);
        const gint haab_index =
            (gint)positive_modulo(calendar_i64_add_saturating(total_kin, 348), MAYAN_KIN_PER_HAAB);
        const gint haab_month = haab_index < 360 ? haab_index / 20 : 18;
        const gint haab_day =
            haab_index < 360 ? haab_index % 20 : haab_index - 360;

        return g_strdup_printf(
            "%" G_GINT64_FORMAT ".%" G_GINT64_FORMAT
            ".%" G_GINT64_FORMAT ".%d.%d · %d %s · %d %s",
            baktun, katun, tun, fields->month, fields->day,
            tzolkin_number, mayan_tzolkin_names[tzolkin_name],
            haab_day, mayan_haab_months[haab_month]);
    }
}
