/*
 * Common test code
 * $Id: grammar.js 561 2003-12-04 10:39:04Z d $
 */

/*
 * Tests scripts should first call describe(), then test() for each test, 
 * and then call finish() at the end.
 */

var failures = 0;
var total = 0;
var saved_desc;

/** Describes the current test. Should be the first function called! */
function describe(desc) {
	print()
	print("===============================")
	print("Test: " + desc)
	print()
	saved_desc = desc;
}

/* Literalise a string for printing purposes */
function literal(v) {
    if (v === NO_EXCEPTION) return "NO_EXCEPTION";
    if (v === ANY_EXCEPTION) return "ANY_EXCEPTION";
    try {
	switch (typeof v) {
	case "string":
		var t = '"' + v.replace(/[\\'"]/g, "\\$&") + '"';
		var s = "";
		for (var i = 0; i < t.length; i++) {
		    var c = t.charCodeAt(i);
		    if (c == '\n'.charCodeAt(0)) s = s + "\\n";
		    else if (c == '\t'.charCodeAt(0)) s = s + "\\t";
		    else if (c == '\r'.charCodeAt(0)) s = s + "\\r";
		    else if (c < 16) s = s + "\\x0" + c.toString(16);
		    else if (c < 32) s = s + "\\x" + c.toString(16);
		    else if (c < 127) s = s + t.charAt(i);
		    else if (c < 0x100) s = s + "\\x" + c.toString(16);
		    else if (c < 0x1000) s = s + "\\u0" + c.toString(16);
		    else s = s + "\\u" + c.toString(16);
		}
		return s;
	default:
		return String(v);
	}
    } catch (e) {
	return "<cannot represent " + typeof v + " value as string>";
    }
}

/*
 * instances of ExceptionClass are treated specially by the test() function.
 * They only match values that have been thrown.
 */
function Exception(value) { 
    switch (value) {
    /* For error classes, we match by constructor only */
    case Error:	case EvalError: case RangeError:
    case ReferenceError: case SyntaxError: case TypeError: case URIError:
	return new ExceptionInstance(value);
    /* Everything else we match exactly */
    default:
	return new ExceptionValue(value); 
    }
}

/* Exception base class */
function ExceptionBase() {}
ExceptionBase.prototype = {}

/* Exceptions that match a particular value (usually not an object) */
function ExceptionValue(value) { 
    this.value = value; }
ExceptionValue.prototype = new ExceptionBase();
ExceptionValue.prototype.matches = function(v) { 
    return this.value == v
};
ExceptionValue.prototype.toString = function() { 
    return "throw " + literal(this.value);
};

/* Exceptions that match when they are an instance of a error class */
function ExceptionInstance(base) { 
    this.base = base; }
ExceptionInstance.prototype = new ExceptionBase();
ExceptionInstance.prototype.toString = function() { 
    return "throw " + this.base.prototype.name + "(...)";
};
ExceptionInstance.prototype.matches = function(v) { 
    /* Strict ECMA forbids Error.[[HasInstance]] so we have to hack */
    return (typeof v == "object") && v && v.constructor == this.base;
};

var NO_EXCEPTION = {}
var ANY_EXCEPTION = {}


/* Indicates the successful conculsion of a test */
function pass(msg) {
	if (Shell.isatty())
	    print(msg + " - [32mPASS[m");
	else
	    print(msg + " - PASS");
	total++;
}

/* Indicates the unsuccessful conclusion of a test */
function fail(msg, extra) {
	var s;
	if (Shell.isatty())
	    s = msg + " - [31;7mFAIL[m";
	else
	    s = msg + " - FAIL";
	if (extra) s += "\n\t\t(" + extra + ")";
	print(s);
	failures++;
	total++;
}

/* Evaluates a JS expression, checks that the result is that expected
 * and calls either pass() or fail(). */
function test(expr, expected) {

	var result, msg, ok, result_str;

	try {
		result = eval(expr);
		if (expected instanceof ExceptionBase)
		    ok = false;
		else if (expected === NO_EXCEPTION)
		    ok = true;
		else if (typeof expected == "number" && isNaN(expected))
		    ok = typeof result == "number" && isNaN(result);
		else
		    ok = (result === expected);
		result_str = literal(result);
	} catch (e) {
		if (expected === ANY_EXCEPTION)
		    ok = true;
		else if (expected instanceof ExceptionBase)
		    ok = expected.matches(e);
		else
		    ok = false;
		result_str = "throw " + literal(e);
	}

	msg = expr + ' = ' + result_str;
	if (ok) {
		pass(msg);
	} else {
		fail(msg, "expected " + literal(expected));
	}
}

/* Displays a summary of the test passes and failures, and throws a
 * final Error if there were any failures. */
function finish() {
	print();
	print("End: " + saved_desc);
	print("     " + (total - failures) + " of " + 
		      total + " sub-tests passed");
	print("===============================")

	/* Throw an error on failure */
	if (failures > 0)
		throw new Error("tests failure");
}

/* Returns the class of an object */
function getClass(v) {
    return /^\[object (.*)\]$/.exec(Object.prototype.toString.apply(v))[1];
}

