
describe("Object object tests")

/* Renders the string form of a value, recursively displaying */
var TEST_RECURSE = "$TEST_RECURSE";
function _(v) {
    if (typeof v != "object")
	return literal(v);
    if (v === null) 
	return "null";
    if (v.hasOwnProperty(TEST_RECURSE))
	return "...";
    try {
	v[TEST_RECURSE] = true;
	var s = "{ [Class]: " + literal(getClass(v));
	for (var p in v)
	    if (p != TEST_RECURSE)
		s = s + ',' + p + ': ' + _(v[p]);
	return s + " }";
    } finally {
	delete v[TEST_RECURSE];
    }
}

var emptyObject = '{ [Class]: "Object" }'
/* 15.2.1.1 */
test("_(Object(null))", emptyObject)
test("_(Object(undefined))", emptyObject)
test("_(Object())", emptyObject)
/* ToObject(boolean) */
test("getClass(Object(true))", "Boolean")
test("Object(true).valueOf()", true)
test("getClass(Object(false))", "Boolean")
test("Object(false).valueOf()", false)
/* ToObject(number) */
test("getClass(Object(123))", "Number")
test("Object(123).valueOf()", 123)
/* ToObject(string) */
test("getClass(Object('foo'))", "String")
test("Object('foo').valueOf()", 'foo')
/* ToObject(object) */
test("var o = {a:1}; Object(o) === o", true)

/* 15.2.2.1 */
test("_(new Object())", emptyObject)
test("_(new Object(null))", emptyObject)
test("_(new Object(undefined))", emptyObject)
test("new Object(Object) === Object", true)
test("new Object(Boolean) === Boolean", true)
/* ToObject(boolean) */
test("getClass(new Object(true))", "Boolean")
test("(new Object(true)).valueOf()", true)
test("getClass(new Object(false))", "Boolean")
test("(new Object(false)).valueOf()", false)
/* ToObject(number) */
test("getClass(new Object(123))", "Number")
test("(new Object(123)).valueOf()", 123)
/* ToObject(string) */
test("getClass(new Object('foo'))", "String")
test("(new Object('foo')).valueOf()", 'foo')

/* 15.2.3 */
test("Object.length", 1)
/* 15.2.3.1 */
test("Object.prototype.constructor === Object", true)

/* 15.2.4.2 */
test("Object.prototype.toString.apply({})", "[object Object]")

/* 15.2.4.5 */
test("({}).hasOwnProperty('p')", false)
test("({p:false}).hasOwnProperty('p')", true)
test("({}).hasOwnProperty('toString')", false)
test("Object.hasOwnProperty('length')", true)

/* 15.2.4.6 */
test("Object.prototype.isPrototypeOf({})", true)
test("Number.prototype.isPrototypeOf({})", false)
test("Object.prototype.isPrototypeOf(new Number(123))", true)

/* 15.2.4.7 */
test("({p:false}).propertyIsEnumerable('p')", true)
test("Object.propertyIsEnumerable('length')", false)

finish()
