/*
 * Copyright (c) 2021, Idan Horowitz <idan.horowitz@serenityos.org>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include <AK/CharacterTypes.h>
#include <AK/DateTimeLexer.h>
#include <LibJS/Runtime/Temporal/AbstractOperations.h>
#include <LibJS/Runtime/Temporal/PlainDate.h>
#include <LibJS/Runtime/Temporal/PlainTime.h>
#include <LibJS/Runtime/Temporal/TimeZone.h>

namespace JS::Temporal {

// 13.34 ParseISODateTime ( isoString ), https://tc39.es/proposal-temporal/#sec-temporal-parseisodatetime
Optional<ISODateTime> parse_iso_date_time(GlobalObject& global_object, [[maybe_unused]] String const& iso_string)
{
    auto& vm = global_object.vm();

    // 1. Assert: Type(isoString) is String.

    // 2. Let year, month, day, hour, minute, second, fraction, and calendar be the parts of isoString produced respectively by the DateYear, DateMonth, DateDay, TimeHour, TimeMinute, TimeSecond, TimeFractionalPart, and CalendarName productions, or undefined if not present.
    Optional<StringView> year_part;
    Optional<StringView> month_part;
    Optional<StringView> day_part;
    Optional<StringView> hour_part;
    Optional<StringView> minute_part;
    Optional<StringView> second_part;
    Optional<StringView> fraction_part;
    Optional<StringView> calendar_part;
    TODO();
    // 3. Let year be the part of isoString produced by the DateYear production.
    // 4. If the first code unit of year is 0x2212 (MINUS SIGN), replace it with the code unit 0x002D (HYPHEN-MINUS).
    String normalized_year;
    if (year_part.has_value() && year_part->starts_with("\xE2\x88\x92"sv))
        normalized_year = String::formatted("-{}", year_part->substring_view(3));
    else
        normalized_year = year_part.value_or("");

    // 5. Set year to ! ToIntegerOrInfinity(year).
    i32 year = Value(js_string(vm, normalized_year)).to_integer_or_infinity(global_object);

    i32 month;
    // 6. If month is undefined, then
    if (!month_part.has_value()) {
        // a. Set month to 1.
        month = 1;
    }
    // 7. Else,
    else {
        // a. Set month to ! ToIntegerOrInfinity(month).
        month = Value(js_string(vm, *month_part)).to_integer_or_infinity(global_object);
    }

    i32 day;
    // 8. If day is undefined, then
    if (!day_part.has_value()) {
        // a. Set day to 1.
        day = 1;
    }
    // 9. Else,
    else {
        // a. Set day to ! ToIntegerOrInfinity(day).
        day = Value(js_string(vm, *day_part)).to_integer_or_infinity(global_object);
    }

    // 10. Set hour to ! ToIntegerOrInfinity(hour).
    i32 hour = Value(js_string(vm, hour_part.value_or(""sv))).to_integer_or_infinity(global_object);

    // 11. Set minute to ! ToIntegerOrInfinity(minute).
    i32 minute = Value(js_string(vm, minute_part.value_or(""sv))).to_integer_or_infinity(global_object);

    // 12. Set second to ! ToIntegerOrInfinity(second).
    i32 second = Value(js_string(vm, second_part.value_or(""sv))).to_integer_or_infinity(global_object);

    // 13. If second is 60, then
    if (second == 60) {
        // a. Set second to 59.
        second = 59;
    }

    i32 millisecond;
    i32 microsecond;
    i32 nanosecond;
    // 14. If fraction is not undefined, then
    if (fraction_part.has_value()) {
        // a. Set fraction to the string-concatenation of the previous value of fraction and the string "000000000".
        auto fraction = String::formatted("{}000000000", *fraction_part);
        // b. Let millisecond be the String value equal to the substring of fraction from 0 to 3.
        // c. Set millisecond to ! ToIntegerOrInfinity(millisecond).
        millisecond = Value(js_string(vm, fraction.substring(0, 3))).to_integer_or_infinity(global_object);
        // d. Let microsecond be the String value equal to the substring of fraction from 3 to 6.
        // e. Set microsecond to ! ToIntegerOrInfinity(microsecond).
        microsecond = Value(js_string(vm, fraction.substring(3, 3))).to_integer_or_infinity(global_object);
        // f. Let nanosecond be the String value equal to the substring of fraction from 6 to 9.
        // g. Set nanosecond to ! ToIntegerOrInfinity(nanosecond).
        nanosecond = Value(js_string(vm, fraction.substring(6, 3))).to_integer_or_infinity(global_object);
    }
    // 15. Else,
    else {
        // a. Let millisecond be 0.
        millisecond = 0;
        // b. Let microsecond be 0.
        microsecond = 0;
        // c. Let nanosecond be 0.
        nanosecond = 0;
    }

    // 16. If ! IsValidISODate(year, month, day) is false, throw a RangeError exception.
    if (!is_valid_iso_date(year, month, day)) {
        vm.throw_exception<RangeError>(global_object, ErrorType::TemporalInvalidISODate);
        return {};
    }

    // 17. If ! IsValidTime(hour, minute, second, millisecond, microsecond, nanosecond) is false, throw a RangeError exception.
    if (!is_valid_time(hour, minute, second, millisecond, microsecond, nanosecond)) {
        vm.throw_exception<RangeError>(global_object, ErrorType::TemporalInvalidTime);
        return {};
    }

    // 18. Return the new Record { [[Year]]: year, [[Month]]: month, [[Day]]: day, [[Hour]]: hour, [[Minute]]: minute, [[Second]]: second, [[Millisecond]]: millisecond, [[Microsecond]]: microsecond, [[Nanosecond]]: nanosecond, [[Calendar]]: calendar }.
    return ISODateTime { .year = year, .month = month, .day = day, .hour = hour, .minute = minute, .second = second, .millisecond = millisecond, .microsecond = microsecond, .nanosecond = nanosecond, .calendar = calendar_part.has_value() ? *calendar_part : Optional<String>() };
}

// 13.35 ParseTemporalInstantString ( isoString ), https://tc39.es/proposal-temporal/#sec-temporal-parsetemporalinstantstring
Optional<TemporalInstant> parse_temporal_instant_string(GlobalObject& global_object, String const& iso_string)
{
    auto& vm = global_object.vm();

    // 1. Assert: Type(isoString) is String.

    // 2. If isoString does not satisfy the syntax of a TemporalInstantString (see 13.33), then
    // a. Throw a RangeError exception.
    // TODO

    // 3. Let result be ! ParseISODateTime(isoString).
    auto result = parse_iso_date_time(global_object, iso_string);

    // 4. Let timeZoneResult be ? ParseTemporalTimeZoneString(isoString).
    auto time_zone_result = parse_temporal_time_zone_string(global_object, iso_string);
    if (vm.exception())
        return {};

    // 5. Assert: timeZoneResult.[[OffsetString]] is not undefined.
    VERIFY(time_zone_result->offset.has_value());

    // 6. Return the new Record { [[Year]]: result.[[Year]], [[Month]]: result.[[Month]], [[Day]]: result.[[Day]], [[Hour]]: result.[[Hour]], [[Minute]]: result.[[Minute]], [[Second]]: result.[[Second]], [[Millisecond]]: result.[[Millisecond]], [[Microsecond]]: result.[[Microsecond]], [[Nanosecond]]: result.[[Nanosecond]], [[TimeZoneOffsetString]]: timeZoneResult.[[OffsetString]] }.
    return TemporalInstant { .year = result->year, .month = result->month, .day = result->day, .hour = result->hour, .minute = result->minute, .second = result->second, .millisecond = result->millisecond, .microsecond = result->microsecond, .nanosecond = result->nanosecond, .time_zone_offset = move(time_zone_result->offset) };
}

// 13.43 ParseTemporalTimeZoneString ( isoString ), https://tc39.es/proposal-temporal/#sec-temporal-parsetemporaltimezonestring
Optional<TemporalTimeZone> parse_temporal_time_zone_string(GlobalObject& global_object, [[maybe_unused]] String const& iso_string)
{
    auto& vm = global_object.vm();
    // 1. Assert: Type(isoString) is String.

    // 2. If isoString does not satisfy the syntax of a TemporalTimeZoneString (see 13.33), then
    // a. Throw a RangeError exception.
    // 3. Let z, sign, hours, minutes, seconds, fraction and name be the parts of isoString produced respectively by the UTCDesignator, TimeZoneUTCOffsetSign, TimeZoneUTCOffsetHour, TimeZoneUTCOffsetMinute, TimeZoneUTCOffsetSecond, TimeZoneUTCOffsetFraction, and TimeZoneIANAName productions, or undefined if not present.
    Optional<StringView> z_part;
    Optional<StringView> sign_part;
    Optional<StringView> hours_part;
    Optional<StringView> minutes_part;
    Optional<StringView> seconds_part;
    Optional<StringView> fraction_part;
    Optional<StringView> name_part;
    TODO();

    // 4. If z is not undefined, then
    if (z_part.has_value()) {
        // a. Return the new Record: { [[Z]]: "Z", [[OffsetString]]: "+00:00", [[Name]]: undefined }.
        return TemporalTimeZone { .z = true, .offset = "+00:00", .name = {} };
    }

    Optional<String> offset;
    // 5. If hours is undefined, then
    if (!hours_part.has_value()) {
        // a. Let offsetString be undefined.
        // NOTE: No-op.
    }
    // 6. Else,
    else {
        // a. Assert: sign is not undefined.
        VERIFY(sign_part.has_value());

        // b. Set hours to ! ToIntegerOrInfinity(hours).
        i32 hours = Value(js_string(vm, *hours_part)).to_integer_or_infinity(global_object);

        i32 sign;
        // c. If sign is the code unit 0x002D (HYPHEN-MINUS) or the code unit 0x2212 (MINUS SIGN), then
        if (sign_part->is_one_of("-", "\u2212")) {
            // i. Set sign to −1.
            sign = -1;
        }
        // d. Else,
        else {
            // i. Set sign to 1.
            sign = 1;
        }

        // e. Set minutes to ! ToIntegerOrInfinity(minutes).
        i32 minutes = Value(js_string(vm, minutes_part.value_or(""sv))).to_integer_or_infinity(global_object);

        // f. Set seconds to ! ToIntegerOrInfinity(seconds).
        i32 seconds = Value(js_string(vm, seconds_part.value_or(""sv))).to_integer_or_infinity(global_object);

        i32 nanoseconds;
        // g. If fraction is not undefined, then
        if (fraction_part.has_value()) {
            // i. Set fraction to the string-concatenation of the previous value of fraction and the string "000000000".
            auto fraction = String::formatted("{}000000000", *fraction_part);
            // ii. Let nanoseconds be the String value equal to the substring of fraction from 0 to 9.
            // iii. Set nanoseconds to ! ToIntegerOrInfinity(nanoseconds).
            nanoseconds = Value(js_string(vm, fraction.substring(0, 9))).to_integer_or_infinity(global_object);
        }
        // h. Else,
        else {
            // i. Let nanoseconds be 0.
            nanoseconds = 0;
        }
        // i. Let offsetNanoseconds be sign × (((hours × 60 + minutes) × 60 + seconds) × 10^9 + nanoseconds).
        auto offset_nanoseconds = sign * (((hours * 60 + minutes) * 60 + seconds) * 1000000000 + nanoseconds);
        // j. Let offsetString be ! FormatTimeZoneOffsetString(offsetNanoseconds).
        offset = format_time_zone_offset_string(offset_nanoseconds);
    }

    Optional<String> name;
    // 7. If name is not undefined, then
    if (name_part.has_value()) {
        // a. If ! IsValidTimeZoneName(name) is false, throw a RangeError exception.
        if (!is_valid_time_zone_name(*name_part)) {
            vm.throw_exception<RangeError>(global_object, ErrorType::TemporalInvalidTimeZoneName);
            return {};
        }
        // b. Set name to ! CanonicalizeTimeZoneName(name).
        name = canonicalize_time_zone_name(*name_part);
    }

    // 8. Return the new Record: { [[Z]]: undefined, [[OffsetString]]: offsetString, [[Name]]: name }.
    return TemporalTimeZone { .z = false, .offset = offset, .name = name };
}

}
