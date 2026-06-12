#pragma once
#include "Evaluator.h"

namespace CyberTracker {

inline void runEvaluatorTests()
{
    int passCount = 0, failCount = 0;

    QVariantMap fieldValues = {
        { "myField", "fieldResult" },
        { "a",       3             },
        { "b",       7             },
    };

    QVariantMap symbols = {
        { "myVar",   10    },
        { "order",   "abc" },   // must NOT tokenize as keyword 'or'
        { "android", "yes" },   // must NOT tokenize as keyword 'and'
        { "divide",  5     },   // must NOT tokenize as keyword 'div'
    };

    QVariantMap variables = {
        { "x", 100 },
        { "y", 200 },
    };

    auto queryFieldValue = [&](const QString& /*recordUid*/, const QString& fieldUid) -> QVariant {
        return fieldValues.value(fieldUid);
    };

    Evaluator eval(&symbols, &variables, queryFieldValue);

    // -------------------------------------------------------------------------
    // Helpers
    // -------------------------------------------------------------------------

    // Verify result is approximately a specific double.
    auto checkDouble = [&](const QString& expression, double expected,
                           const QString& recUid = "", const QString& fieldUid = "")
    {
        auto result = eval.evaluate(expression, recUid, fieldUid);
        bool pass = !eval.getParseError() && qAbs(result.toDouble() - expected) < 0.0001;
        if (!pass)
            qWarning() << "FAIL [double]" << expression << "| expected" << expected << "| got" << result << (eval.getParseError() ? "(parse error)" : "");
        pass ? passCount++ : failCount++;
    };

    // Verify result converts to a specific bool.
    auto checkBool = [&](const QString& expression, bool expected,
                         const QString& recUid = "", const QString& fieldUid = "")
    {
        auto result = eval.evaluate(expression, recUid, fieldUid);
        bool pass = !eval.getParseError() && result.toBool() == expected;
        if (!pass)
            qWarning() << "FAIL [bool]" << expression << "| expected" << expected << "| got" << result << (eval.getParseError() ? "(parse error)" : "");
        pass ? passCount++ : failCount++;
    };

    // Verify result is a specific string.
    auto checkString = [&](const QString& expression, const QString& expected,
                           const QString& recUid = "", const QString& fieldUid = "")
    {
        auto result = eval.evaluate(expression, recUid, fieldUid);
        bool pass = !eval.getParseError() && result.toString() == expected;
        if (!pass)
            qWarning() << "FAIL [string]" << expression << "| expected" << expected << "| got" << result << (eval.getParseError() ? "(parse error)" : "");
        pass ? passCount++ : failCount++;
    };

    // Verify result is a non-empty string without a parse error.
    auto checkNonEmpty = [&](const QString& expression)
    {
        auto result = eval.evaluate(expression, "", "");
        bool pass = !eval.getParseError() && !result.toString().isEmpty();
        if (!pass)
            qWarning() << "FAIL [non-empty]" << expression << "| got" << result << (eval.getParseError() ? "(parse error)" : "");
        pass ? passCount++ : failCount++;
    };

    // Verify result is a valid (non-null) QVariant without a parse error.
    auto checkValid = [&](const QString& expression)
    {
        auto result = eval.evaluate(expression, "", "");
        bool pass = !eval.getParseError() && result.isValid();
        if (!pass)
            qWarning() << "FAIL [valid]" << expression << "| got" << result << (eval.getParseError() ? "(parse error)" : "");
        pass ? passCount++ : failCount++;
    };

    // Verify the expression causes a parse error (e.g. not-implemented functions
    // that return false from the function table, or unknown symbols).
    auto checkError = [&](const QString& expression)
    {
        eval.evaluate(expression, "", "");
        bool pass = eval.getParseError();
        if (!pass)
            qWarning() << "FAIL [expected parse error]" << expression;
        pass ? passCount++ : failCount++;
    };

    // Verify the expression succeeds but returns an invalid (null) QVariant.
    // Used for not-implemented stubs that return true but don't set a result.
    auto checkInvalid = [&](const QString& expression)
    {
        auto result = eval.evaluate(expression, "", "");
        bool pass = !eval.getParseError() && !result.isValid();
        if (!pass)
            qWarning() << "FAIL [expected invalid variant]" << expression << "| got" << result << (eval.getParseError() ? "(parse error)" : "");
        pass ? passCount++ : failCount++;
    };

    // =========================================================================
    // Numbers
    // =========================================================================

    checkDouble("42",       42.0);
    checkDouble("3.14",     3.14);
    checkDouble("-7",       -7.0);
    checkDouble("+12",      12.0);
    checkDouble("1.01e+12", 1.01e+12);
    checkDouble("1.5e-3",   1.5e-3);
    checkDouble("0",        0.0);

    // =========================================================================
    // Arithmetic
    // =========================================================================

    checkDouble("2 + 3",         5.0);
    checkDouble("10 - 4",        6.0);
    checkDouble("3 * 4",         12.0);
    checkDouble("10 / 4",        2.5);
    checkDouble("10 div 4",      2.5);   // div is floating-point division (same as /)
    checkDouble("-5 + 10",       5.0);
    checkDouble("(2 + 3) * 4",   20.0);
    checkDouble("2 + 3 * 4",     14.0);  // * binds tighter than +
    checkDouble("-(3 + 4)",      -7.0);
    checkDouble("10 / 0",        0.0);   // divide by zero yields 0

    // =========================================================================
    // Comparisons
    // =========================================================================

    checkBool("1 = 1",   true);   // = uses compareVariants
    checkBool("1 = 2",   false);
    checkBool("1 == 1",  true);   // == uses fabs path
    checkBool("1 != 2",  true);
    checkBool("1 != 1",  false);
    checkBool("3 < 5",   true);
    checkBool("5 < 3",   false);
    checkBool("5 > 3",   true);
    checkBool("3 > 5",   false);
    checkBool("3 <= 3",  true);
    checkBool("4 <= 3",  false);
    checkBool("4 >= 4",  true);
    checkBool("3 >= 4",  false);

    // String equality via compareVariants (falls through to QVariant::operator==).
    checkBool("'abc' = 'abc'",  true);
    checkBool("'abc' = 'xyz'",  false);
    checkBool("'abc' != 'xyz'", true);
    checkBool("'abc' != 'abc'", false);

    // =========================================================================
    // Logic  (and / or / div require trailing space or '(' per tokeniser)
    // =========================================================================

    checkBool("1 and 1",          true);
    checkBool("1 and 0",          false);
    checkBool("0 and 1",          false);
    checkBool("0 or 1",           true);
    checkBool("0 or 0",           false);
    checkBool("1 or 0",           true);
    checkBool("0 and 1 or 1",     true);    // (0 and 1)=false; (false or 1)=true

    // Keyword immediately before '(' instead of space.
    checkBool("0 or(1)",          true);
    checkBool("1 and(1)",         true);
    checkDouble("10 div(2)",      5.0);

    // Unary not (returns double 1.0 / 0.0).
    checkDouble("!0",             1.0);
    checkDouble("!1",             0.0);
    checkDouble("!!1",            1.0);

    // =========================================================================
    // String literals (single and double quoted)
    // =========================================================================

    checkString("'hello'",    "hello");
    checkString("\"world\"",  "world");

    // =========================================================================
    // Arrays  [ ... ]
    // =========================================================================

    checkDouble("sum([1, 2, 3])",              6.0);
    checkDouble("min([4, 1, 7])",              1.0);
    checkDouble("max([4, 1, 7])",              7.0);
    checkDouble("count([1, 2, 3])",            3.0);
    checkDouble("count-non-empty(['a', '', 'b'])", 2.0);

    // =========================================================================
    // Comma list (returns last value)
    // =========================================================================

    checkDouble("1, 2, 3",        3.0);
    checkString("'a', 'b', 'c'",  "c");

    // =========================================================================
    // Grouping
    // =========================================================================

    checkDouble("(1 + 2) * 3",   9.0);
    checkDouble("((4))",          4.0);

    // =========================================================================
    // Symbol / variable lookup
    // =========================================================================

    // --- Basic retrieval ---
    checkDouble("myVar",    10.0);    // numeric symbol
    checkString("order",    "abc");   // string symbol
    checkDouble("x",        100.0);   // numeric variable
    checkDouble("y",        200.0);   // numeric variable

    // --- Symbols starting with reserved keywords must NOT be split ---
    checkString("order",    "abc");   // starts with 'or'
    checkString("android",  "yes");   // starts with 'and'
    checkDouble("divide",   5.0);     // starts with 'div'

    // --- Symbols in arithmetic ---
    checkDouble("myVar + 5",           15.0);
    checkDouble("myVar - 3",           7.0);
    checkDouble("myVar * 2",           20.0);
    checkDouble("myVar / 4",           2.5);
    checkDouble("myVar + myVar",       20.0);

    // --- Variables in arithmetic ---
    checkDouble("x + y",               300.0);
    checkDouble("x - y",               -100.0);
    checkDouble("x * 2",               200.0);
    checkDouble("y / 4",               50.0);

    // --- Symbols / variables in comparisons ---
    checkBool("myVar = 10",            true);
    checkBool("myVar > 5",             true);
    checkBool("myVar < 5",             false);
    checkBool("myVar >= 10",           true);
    checkBool("myVar <= 9",            false);
    checkBool("myVar != 99",           true);
    checkBool("x = 100",               true);
    checkBool("x > y",                 false);
    checkBool("x < y",                 true);
    checkBool("x != y",                true);

    // --- Symbols / variables as function arguments ---
    checkDouble("abs(myVar)",          10.0);
    checkDouble("sqrt(myVar - 6)",     2.0);     // sqrt(4) = 2
    checkDouble("pow(myVar, 2)",       100.0);
    checkDouble("round(myVar, 0)",     10.0);
    checkDouble("sum([myVar, x])",     110.0);   // 10 + 100

    // --- String symbol in function ---
    checkBool("contains(order, 'ab')",          true);
    checkDouble("string-length(order)",         3.0);
    checkBool("starts-with(order, 'ab')",       true);

    // --- Assign operators: compute symbol_value OP rhs, do NOT modify the symbol ---
    // myVar = 10
    checkDouble("myVar += 5",          15.0);    // 10 + 5
    checkDouble("myVar -= 3",          7.0);     // 10 - 3
    checkDouble("myVar *= 2",          20.0);    // 10 * 2
    checkDouble("myVar /= 4",          2.5);     // 10 / 4
    checkDouble("myVar /= 0",          0.0);     // divide by zero → 0
    // x = 100
    checkDouble("x += 50",             150.0);
    checkDouble("x -= 25",             75.0);
    checkDouble("x *= 3",              300.0);
    checkDouble("x /= 4",              25.0);
    // Assign operators do not persist — myVar is still 10 after all the above.
    checkDouble("myVar",               10.0);
    checkDouble("x",                   100.0);

    // --- Variable takes priority over symbol with the same name ---
    {
        QVariantMap overlapSymbols  = { { "val", 999 } };
        QVariantMap overlapVars     = { { "val",  42 } };
        Evaluator overlapEval(&overlapSymbols, &overlapVars, nullptr);
        auto r = overlapEval.evaluate("val", "", "");
        bool pass = !overlapEval.getParseError() && r.toDouble() == 42.0;
        if (!pass)
            qWarning() << "FAIL [variable priority] val | expected 42 | got" << r;
        pass ? passCount++ : failCount++;
    }

    // --- Symbol-only lookup (no variables map) ---
    {
        QVariantMap onlySymbols = { { "pi_approx", 3.14 } };
        Evaluator symOnlyEval(&onlySymbols, nullptr, nullptr);
        auto r = symOnlyEval.evaluate("pi_approx", "", "");
        bool pass = !symOnlyEval.getParseError() && qAbs(r.toDouble() - 3.14) < 0.0001;
        if (!pass)
            qWarning() << "FAIL [symbol-only] pi_approx | expected 3.14 | got" << r;
        pass ? passCount++ : failCount++;
    }

    // --- Missing field value falls back to invalid QVariant; toDouble() gives 0 ---
    checkDouble("${notExist} + 5",     5.0,   "rec1", "");   // invalid → 0 + 5

    // =========================================================================
    // Field values:  ${fieldUid}  and  .  (current field)
    // =========================================================================

    checkString("${myField}",                  "fieldResult", "rec1", "");
    checkDouble("${a} + ${b}",                 10.0,          "rec1", "");   // 3 + 7
    checkBool("${myField} = 'fieldResult'",    true,          "rec1", "");

    checkString(".",                           "fieldResult", "rec1", "myField");
    checkBool(". = 'fieldResult'",             true,          "rec1", "myField");

    // =========================================================================
    // if(condition, then, else)
    // =========================================================================

    checkString("if(1, 'yes', 'no')",          "yes");
    checkString("if(0, 'yes', 'no')",          "no");
    checkDouble("if(1 = 1, 42, 0)",            42.0);
    checkDouble("if(1 = 2, 42, 0)",            0.0);
    checkBool("if(true(), true(), false())",   true);
    checkBool("if(false(), true(), false())",  false);

    // =========================================================================
    // once(expression)  — returns its argument unchanged
    // =========================================================================

    checkDouble("once(99)",       99.0);
    checkString("once('hello')",  "hello");

    // =========================================================================
    // position(xpath)  — stub: succeeds but returns invalid QVariant
    // =========================================================================

    //checkInvalid("position('')");

    // =========================================================================
    // selected(space_delimited_array, string)
    // =========================================================================

    checkBool("selected('a b c', 'a')",   true);
    checkBool("selected('a b c', 'b')",   true);
    checkBool("selected('a b c', 'c')",   true);
    checkBool("selected('a b c', 'z')",   false);
    checkBool("selected('', 'a')",        false);

    // =========================================================================
    // selected-at(space_delimited_array, n)
    // =========================================================================

    checkString("selected-at('a b c', 0)", "a");
    checkString("selected-at('a b c', 1)", "b");
    checkString("selected-at('a b c', 2)", "c");

    // =========================================================================
    // indexed-repeat(...)  — not implemented: function returns false → parse error
    // =========================================================================

    //checkError("indexed-repeat('a', 'b', 1)");

    // =========================================================================
    // count-selected(multi_select_question)
    // =========================================================================

    checkDouble("count-selected('a b c')",  3.0);
    checkDouble("count-selected('a')",      1.0);
    checkDouble("count-selected('')",       1.0);  // ''.split(' ') has 1 element

    // =========================================================================
    // count(nodeset)
    // =========================================================================

    checkDouble("count([1, 2, 3])",          3.0);
    checkDouble("count([1])",                1.0);

    // =========================================================================
    // count-non-empty(nodeset)
    // =========================================================================

    checkDouble("count-non-empty(['a', 'b', 'c'])",    3.0);
    checkDouble("count-non-empty(['a', '', 'c'])",     2.0);
    checkDouble("count-non-empty(['', '', ''])",       0.0);

    // =========================================================================
    // sum(nodeset)
    // =========================================================================

    checkDouble("sum([1, 2, 3])",       6.0);
    checkDouble("sum([10, -3, 0.5])",   7.5);

    // =========================================================================
    // max(nodeset)
    // =========================================================================

    checkDouble("max([3, 1, 4, 1, 5])", 5.0);
    checkDouble("max([-1, -2, -3])",    -1.0);

    // =========================================================================
    // min(nodeset)
    // =========================================================================

    checkDouble("min([3, 1, 4, 1, 5])", 1.0);
    checkDouble("min([-1, -2, -3])",    -3.0);

    // =========================================================================
    // regex(string, expression)
    // =========================================================================

    checkBool("regex('hello world', 'hello.*')",    true);
    checkBool("regex('hello', '^world')",           false);
    checkBool("regex('12345', '[0-9]+')",           true);
    checkBool("regex('abc123', '[a-z]+')",          true);   // partial match
    checkBool("regex('', '.*')",                    true);   // empty string matches .*
    checkBool("regex('hello', '^hello$')",          true);

    // =========================================================================
    // contains(string, substring)
    // =========================================================================

    checkBool("contains('foobar', 'oba')",    true);
    checkBool("contains('foobar', 'xyz')",    false);
    checkBool("contains('foobar', '')",       true);
    checkBool("contains('', 'x')",            false);

    // =========================================================================
    // starts-with(string, substring)
    // =========================================================================

    checkBool("starts-with('hello', 'hel')",   true);
    checkBool("starts-with('hello', 'ell')",   false);
    checkBool("starts-with('hello', '')",      true);
    checkBool("starts-with('', 'x')",          false);

    // =========================================================================
    // ends-with(string, substring)
    // =========================================================================

    checkBool("ends-with('hello', 'llo')",     true);
    checkBool("ends-with('hello', 'hel')",     false);
    checkBool("ends-with('hello', '')",        true);
    checkBool("ends-with('', 'x')",            false);

    // =========================================================================
    // substr(string, start[, end])
    // =========================================================================

    checkString("substr('hello', 0)",       "hello");
    checkString("substr('hello', 1)",       "ello");
    checkString("substr('hello', 1, 3)",    "ell");
    checkString("substr('hello', 0, 3)",    "hel");

    // =========================================================================
    // substring-before(string, target)
    // =========================================================================

    checkString("substring-before('hello world', ' ')",   "hello");
    checkString("substring-before('a/b/c', '/')",         "a");
    checkString("substring-before('hello', 'xyz')",       "");   // not found → ""

    // =========================================================================
    // substring-after(string, target)
    // =========================================================================

    checkString("substring-after('hello world', ' ')",    "world");
    checkString("substring-after('a/b/c', '/')",          "b/c");
    checkString("substring-after('hello', 'xyz')",        "");    // not found → ""

    // =========================================================================
    // translate(string, from, to)  — not implemented: parse error
    // =========================================================================

    //checkError("translate('abc', 'a', 'x')");

    // =========================================================================
    // string-length(string)
    // =========================================================================

    checkDouble("string-length('hello')",     5.0);
    checkDouble("string-length('')",          0.0);
    checkDouble("string-length('a b c')",     5.0);

    // =========================================================================
    // normalize-space(string)
    // =========================================================================

    checkString("normalize-space('  a  b  c  ')", "a b c");
    checkString("normalize-space('hello')",        "hello");
    checkString("normalize-space('  ')",           "");

    // =========================================================================
    // concat(arg [, arg ...])
    // =========================================================================

    checkString("concat('foo', 'bar')",          "foobar");
    checkString("concat('a', 'b', 'c')",         "abc");
    checkString("concat('hello', ' ', 'world')", "hello world");
    checkString("concat('', 'x')",               "x");

    // =========================================================================
    // join(separator, nodeset)
    // =========================================================================

    checkString("join('-', ['a', 'b', 'c'])",    "a-b-c");
    checkString("join(', ', ['x', 'y'])",        "x, y");
    checkString("join('-', ['a'])",              "a");

    // =========================================================================
    // boolean-from-string(string)
    // =========================================================================

    checkBool("boolean-from-string('true')",     true);
    checkBool("boolean-from-string('1')",        true);
    checkBool("boolean-from-string('false')",    false);
    checkBool("boolean-from-string('0')",        false);
    checkBool("boolean-from-string('yes')",      false);  // only "true" and "1" are true

    // =========================================================================
    // string(arg)
    // =========================================================================

    checkString("string(42)",       "42");
    checkString("string(3.14)",     "3.14");
    checkString("string('hello')",  "hello");

    // =========================================================================
    // round(number, places)
    // =========================================================================

    checkDouble("round(3.14159, 2)",   3.14);
    checkDouble("round(3.14159, 4)",   3.1416);
    checkDouble("round(2.555, 2)",     2.56);
    checkDouble("round(2.5, 0)",       3.0);
    checkDouble("round(-1.5, 0)",      -2.0);

    // =========================================================================
    // int(number)  — truncates toward zero via static_cast<int>
    // =========================================================================

    checkDouble("int(3.9)",     3.0);
    checkDouble("int(3.1)",     3.0);
    checkDouble("int(-3.9)",   -3.0);
    checkDouble("int(0.9)",     0.0);

    // =========================================================================
    // number(arg)
    // =========================================================================

    checkDouble("number('42')",      42.0);
    checkDouble("number('3.14')",    3.14);
    checkDouble("number('-7')",      -7.0);

    // =========================================================================
    // digest(data, algorithm[, encoding])
    // Known test vectors for empty-string input.
    // =========================================================================

    // MD5("") hex = d41d8cd98f00b204e9800998ecf8427e
    checkString("digest('', 'MD5', 'hex')",    "d41d8cd98f00b204e9800998ecf8427e");
    // MD5("") base64 = 1B2M2Y8AsgTpgAmY7PhCfg==
    checkString("digest('', 'MD5', 'base64')", "1B2M2Y8AsgTpgAmY7PhCfg==");
    // SHA-1("") hex = da39a3ee5e6b4b0d3255bfef95601890afd80709
    checkString("digest('', 'SHA-1', 'hex')",  "da39a3ee5e6b4b0d3255bfef95601890afd80709");
    // SHA-256("") hex = e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855
    checkString("digest('', 'SHA-256', 'hex')", "e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855");
    // Default encoding is base64.
    checkString("digest('', 'MD5')",           "1B2M2Y8AsgTpgAmY7PhCfg==");
    // Unknown encoding → empty string.
    checkString("digest('', 'MD5', 'unknown')", "");

    // =========================================================================
    // pow(number, power)
    // =========================================================================

    checkDouble("pow(2, 10)",     1024.0);
    checkDouble("pow(9, 0.5)",    3.0);
    checkDouble("pow(10, 0)",     1.0);
    checkDouble("pow(2, -1)",     0.5);

    // =========================================================================
    // log(number)  — natural logarithm
    // =========================================================================

    checkDouble("log(1)",        0.0);
    checkDouble("log(exp(1))",   1.0);

    // =========================================================================
    // log10(number)
    // =========================================================================

    checkDouble("log10(1)",      0.0);
    checkDouble("log10(10)",     1.0);
    checkDouble("log10(100)",    2.0);
    checkDouble("log10(1000)",   3.0);

    // =========================================================================
    // abs(number)
    // =========================================================================

    checkDouble("abs(-5)",      5.0);
    checkDouble("abs(5)",       5.0);
    checkDouble("abs(0)",       0.0);
    checkDouble("abs(-0.001)",  0.001);

    // =========================================================================
    // sin / cos / tan (radians)
    // =========================================================================

    checkDouble("sin(0)",                         0.0);
    checkDouble("cos(0)",                         1.0);
    checkDouble("tan(0)",                         0.0);
    checkDouble("sin(pi() / 2)",                  1.0);
    checkDouble("cos(pi())",                      -1.0);
    checkDouble("sin(pi())",                      0.0);   // approx 0

    // =========================================================================
    // asin / acos / atan / atan2
    // =========================================================================

    checkDouble("asin(0)",       0.0);
    checkDouble("asin(1)",       M_PI / 2);
    checkDouble("acos(1)",       0.0);
    checkDouble("acos(0)",       M_PI / 2);
    checkDouble("atan(0)",       0.0);
    checkDouble("atan(1)",       M_PI / 4);
    checkDouble("atan2(0, 1)",   0.0);
    checkDouble("atan2(1, 0)",   M_PI / 2);
    checkDouble("atan2(1, 1)",   M_PI / 4);

    // Verify round-trip: asin(sin(x)) = x for x in [-pi/2, pi/2].
    checkDouble("asin(sin(0.5))",               0.5);
    checkDouble("acos(cos(0.5))",               0.5);
    checkDouble("atan(tan(0.5))",               0.5);
    checkDouble("atan2(sin(1.2), cos(1.2))",    1.2);

    // =========================================================================
    // sqrt(number)
    // =========================================================================

    checkDouble("sqrt(16)",     4.0);
    checkDouble("sqrt(2)",      1.4142);
    checkDouble("sqrt(0)",      0.0);
    checkDouble("sqrt(1)",      1.0);

    // =========================================================================
    // exp(x)  — e^x
    // =========================================================================

    checkDouble("exp(0)",       1.0);
    checkDouble("exp(1)",       M_E);
    checkDouble("exp(-1)",      1.0 / M_E);

    // =========================================================================
    // exp10(x)  — 10^x
    // =========================================================================

    checkDouble("exp10(0)",     1.0);
    checkDouble("exp10(1)",     10.0);
    checkDouble("exp10(2)",     100.0);
    checkDouble("exp10(-1)",    0.1);

    // =========================================================================
    // pi()
    // =========================================================================

    checkDouble("pi()",         M_PI);

    // =========================================================================
    // today() / now()
    // Require App::instance()->timeManager() to be initialized.
    // Only check that they return a non-empty string without a parse error.
    // =========================================================================

    checkNonEmpty("today()");
    checkNonEmpty("now()");

    // =========================================================================
    // decimal-date-time(dateTime)
    // Returns days since 1970-01-01 as a double.
    // Uses Utils::decodeTimestampSecs, which is timezone-sensitive.
    // =========================================================================

    checkDouble("decimal-date-time('1970-01-01')", 0.0);

    // =========================================================================
    // date(days)
    // Converts days-since-epoch to an ISO date string.
    // Uses QDateTime::fromSecsSinceEpoch which reflects local timezone.
    // =========================================================================

    // Day 0 in local time must be 1970-01-01 (applies in UTC and positive offsets).
    checkString("date(0)", "1970-01-01");

    // =========================================================================
    // date-time(days)
    // Requires App::instance()->timeManager()->formatDateTime().
    // Only check for non-empty result.
    // =========================================================================

    checkNonEmpty("date-time(0)");

    // =========================================================================
    // decimal-time(time)
    // Returns the time portion as a fraction of a day (0.0 = midnight, 0.5 = noon).
    // =========================================================================

    checkDouble("decimal-time('1970-01-01T00:00:00')", 0.0);
    checkDouble("decimal-time('1970-01-01T12:00:00')", 0.5);

    // =========================================================================
    // format-date(date, format)
    // =========================================================================

    checkString("format-date('2024-06-15', '%Y-%m-%d')",       "2024-06-15");
    checkString("format-date('2024-06-15', '%d/%m/%Y')",       "15/06/2024");
    checkString("format-date('2024-06-15', '%Y')",             "2024");

    // =========================================================================
    // format-date-time(dateTime, format)
    // =========================================================================

    checkString("format-date-time('2024-06-15T09:30:00', '%Y-%m-%d')", "2024-06-15");
    checkString("format-date-time('2024-06-15T09:30:00', '%H:%M')",    "09:30");

    // =========================================================================
    // area(nodeset | geoshape)
    // distance(nodeset | geotrace)
    // Pass an empty string — Utils::pointsFromXml returns no points.
    // Area and distance of zero points is expected to be 0.
    // =========================================================================

    checkDouble("area('')",     0.0);
    checkDouble("distance('')", 0.0);

    // =========================================================================
    // random()
    // Non-deterministic; verify the result is in [0, 1).
    // =========================================================================

    checkBool("random() >= 0",  true);
    checkBool("random() < 1",   true);

    // =========================================================================
    // randomize(nodeset[, seed])
    // Not implemented but returns true — result is a valid (empty?) variant.
    // Just verify it does not cause a parse error.
    // =========================================================================

    //checkInvalid("randomize(['a', 'b', 'c'])");

    // =========================================================================
    // uuid([length])
    // =========================================================================

    checkDouble("string-length(uuid())",   16.0);  // default length = 16
    checkDouble("string-length(uuid(8))",  8.0);
    checkDouble("string-length(uuid(32))", 32.0);

    // =========================================================================
    // boolean(arg)
    // =========================================================================

    checkBool("boolean(1)",        true);
    checkBool("boolean(0)",        false);
    checkBool("boolean('true')",   true);    // non-empty, non-"false", non-"0"
    checkBool("boolean('false')",  false);
    checkBool("boolean('')",       false);

    // =========================================================================
    // not(arg)
    // =========================================================================

    checkBool("not(true())",        false);
    checkBool("not(false())",       true);
    checkBool("not(1)",             false);
    checkBool("not(0)",             true);
    checkBool("not(not(true()))",   true);

    // =========================================================================
    // coalesce(arg1, arg2)
    // =========================================================================

    checkString("coalesce('', 'default')",      "default");
    checkString("coalesce('value', 'default')", "value");
    checkString("coalesce('a', 'b')",           "a");
    checkString("coalesce('', '')",             "");

    // =========================================================================
    // checklist(min, max, ...)  — not implemented: parse error
    // weighted-checklist(min, max, ...)  — not implemented: parse error
    // =========================================================================

    //checkError("checklist(0, 3, 1, 1, 1)");
    //checkError("weighted-checklist(0, 3, 1, 1, 1, 1)");

    // =========================================================================
    // true() / false()
    // =========================================================================

    checkBool("true()",   true);
    checkBool("false()",  false);

    // =========================================================================
    // make_list(arg [, arg ...])
    // Returns a QVariantList — exercise via count() and sum().
    // =========================================================================

    checkDouble("count(make_list('a', 'b', 'c'))",        3.0);
    checkDouble("sum(make_list(1, 2, 3))",                6.0);
    checkDouble("count(make_list(1))",                    1.0);
    checkBool("contains(make_list('x', 'y', 'z'), 'y')", true);  // make_list result is
                                                                   // joined as a string by contains

    // =========================================================================
    // checklist_match(field_values, match_mode, matches)
    // =========================================================================

    // 'and' mode: all requested items must be present.
    checkBool("checklist_match('a b c', 'and', ['a', 'b'])",         true);
    checkBool("checklist_match('a b c', 'and', ['a', 'b', 'c'])",    true);
    checkBool("checklist_match('a b', 'and', ['a', 'b', 'c'])",      false);  // c missing

    // 'or' mode: at least one item must be present.
    checkBool("checklist_match('a b c', 'or', ['x', 'a'])",          true);   // a present
    checkBool("checklist_match('a b c', 'or', ['x', 'y'])",          false);  // neither present

    // 'exact' mode: same items, same count (order-independent).
    checkBool("checklist_match('a b', 'exact', ['a', 'b'])",         true);
    checkBool("checklist_match('a b', 'exact', ['a'])",              false);  // count mismatch
    checkBool("checklist_match('a b', 'exact', ['b', 'a'])",         true);   // order irrelevant
    checkBool("checklist_match('a b c', 'exact', ['a', 'b', 'x'])",  false);  // x not in field

    // =========================================================================
    // Nested / compound expressions
    // =========================================================================

    checkBool("if(1 and 0 or 1, true(), false())",              true);
    checkDouble("abs(-1) + abs(-2)",                            3.0);
    checkBool("string-length('hi') > 1",                       true);
    checkBool("if(string-length('hi') > 1, 1, 0) = 1",         true);
    checkString("concat('val=', string(round(3.14159, 2)))",    "val=3.14");
    checkBool("contains('foobar', 'foo') and 1 = 1",            true);
    checkDouble("(2 + 3) * (4 - 1)",                            15.0);
    checkDouble("sum([pow(2, 1), pow(2, 2), pow(2, 3)])",       14.0);
    checkBool("sqrt(pow(3, 2) + pow(4, 2)) = 5",                true);   // Pythagorean triple

    // =========================================================================
    // Error cases
    // =========================================================================

    checkError("@");              // '@' is not a valid first character
    checkError("unknown_sym");    // not in symbols or variables

    // =========================================================================
    // Summary
    // =========================================================================

    qDebug() << "Evaluator tests:" << passCount << "passed," << failCount << "failed.";
    Q_ASSERT(failCount == 0);
}

} // namespace CyberTracker
