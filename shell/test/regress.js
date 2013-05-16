
describe("Regression tests from bugzilla.")

// Tests the string concatenation optimisation that broke during dev
test('var a = "foo", b = a + "bar"; a', "foo")

function foo () { 1; }
test(foo.prototype.constructor, foo)			/* bug 9 */

var lower = "abcdefghij0123xyz\uff5a";
var upper = "ABCDEFGHIJ0123XYZ\uff3a";
test(literal(lower)+".toUpperCase()", upper)		/* bug 4 */
test(literal(upper)+".toLowerCase()", lower)
test("'foo'.lastIndexOf('o', 0)", -1)			/* bug 24 */
test("typeof asjlkhadlsh", "undefined")

compat('js12')
test("Function().__proto__", Function.prototype)
compat('')

function f() { return this; } 
function g() { var h = f; return h(); } 
test("g()", this)					/* bug 32 */

test("(new String()).indexOf()", -1)			/* bug 33 */
test("-\"\\u20001234\\u2001\"", -1234)			/* r960 */
test("/m/i.ignoreCase == true", true)			/* bug 34 */
test("a: { do { break a } while (true) }",  void 0);	/* bug 35 */
test("/foo/.test('foo')", true);			/* bug 40 */
test("/foo/.test('bar')", false);			/* bug 40 */
test("'foo'.concat('bar')", "foobar");			/* bug 42 */
test("('x'+'y').indexOf('longer')", -1);		/* bug 46 */

/* bug 36 */
function h(x,x) { return arguments[0]+","+arguments[1]; }
test("h(1,2)", "1,2")
function h1(x) { x = 1;                      arguments[0] = 2; return x; }
function h2(x) { x = 1; delete arguments[0]; arguments[0] = 2; return x; }
test("h1()", 1);
test("h1(0)", 2);
test("h2()", 1);
test("h2(0)", 1);

test("var a = 10; a -= NaN; isNaN(a)", true);		/* bug 61 */
test("var b = new Array(1,2); delete b[1];0", 0);	/* bug 62 */

/* bug 66 */
function error_lineno(statement) {
    try { eval(statement); } 
    catch (e) { return Number(/<eval>:(\d+)/.exec(e.message)[1]); }
    return "no error";
}
test('error_lineno("an error")', 1)
test('error_lineno("\\nan error")', 2)
test('error_lineno("/* Comment */\\nan error")', 2)
test('error_lineno("// Comment\\nan error")', 2)
compat('sgmlcom')
test('error_lineno("<!-- Comment\\nan error")', 2)
compat('')

/* bug 68 */
test('var s = ""; for (i = 0; i < 8192; i++) s = s + " "; s.length', 8192)

/* bug 69 (may segfault) */
test('(function(){var v=String.prototype.toLocaleString;v();})()', 
    Exception(TypeError))

/* bug 77 */
test('isFinite(0)', true)
test('isFinite(1000)', true)
test('isFinite(-1000)', true)
test('isFinite(-0)', true)
test('isFinite(Infinity)', false)
test('isFinite(-Infinity)', false)
test('-Infinity < Infinity', true)
test('Infinity < -Infinity', false)
test('isNaN(NaN)', true)
test('NaN == NaN', false)
test('String(Infinity)', "Infinity")
test('String(-Infinity)', "-Infinity")

/* bug 78 */
test('void (function(a){switch(a){case 1:case 2:;}}).toString()', undefined);

/* bug 79 */
delete bar;
compat('sgmlcom')
test('1 --> bar', Exception(SyntaxError));
test(' --> bar', undefined);
test('1 \n --> bar', 1);
test(' /* ... */ --> bar', undefined);
compat('');

/* bug 82 */
test('function b() { return (function () { this.foo = "bar"; }); } '+
     'new (b())() instanceof b()', true);
compat('js14');
test('(function(){}) instanceof Function', true);
test('(function(){}) instanceof Object', true);
test('isNaN instanceof Function', true);
test('({}) instanceof Object', true);
test('Function.__proto__ === Function.prototype', true);    /* assumption */
test('Function.__proto__ instanceof Function', false);
compat('');

/* bug 84 */
test('("foo".match(/x/g)).length', 0);
compat('errata');
test('"foo".match(/x/g)', null);
compat('');

/* bug 91 */
compat('js15')
test("if(true){function a91(){}\n}\n typeof a91", "function");
test("if(true){function b91(){}\n function c91(){}\n}\n typeof b91 + typeof c91", "functionfunction");
test("var x = typeof d91 + typeof e91; function d91(){}\n function e91(){}\nx", "functionfunction")
test("var x = typeof f91; if (true){function f91() {};} x", "undefined")
compat('');
test("var x = typeof g91 + typeof h91; function g91(){}\n function h91(){}\nx", "functionfunction")

/* bug 93 */
test("'abc'.charAt()", 'a')
test("'abc'.charCodeAt()", 'a'.charCodeAt(0))

/* bug 94 */
test("var s=''; for(var i=0;i<35;i++)s+='abcdefghijklmno';s.charCodeAt(0x200)",
    "c".charCodeAt(0));

/* bug 97 */
test('error_lineno("\\r\\nan error")', 2)

/* bug 101 */
test("\"xxx\\\nyyy\"", Exception(SyntaxError))
compat('js11')
test("\"xxx\\\nyyy\"", 'xxxyyy')
compat('')

test("(7e-4).toString(undefined)", "0.0007")		/* bug 108 */
test("var i=1, i\\u002b=2", Exception(SyntaxError))	/* bug 109 */
test("/[]/.exec('')", null)				/* bug 110 */
test("/)/", Exception(SyntaxError))				/* bug 112 */
test("/}/", Exception(SyntaxError))
test("/]/", Exception(SyntaxError))
test("/.**./", Exception(SyntaxError))
test("/+/", Exception(SyntaxError))
test("/?/", Exception(SyntaxError))

/* Bug 115 */
test("Object.prototype.isPrototypeOf(new Number(123))", true);

/* Bug 117 */
test("var functest; (function functest(arg){if(arg)return 1;functest=function(arg){return 2;};return functest(true);})(false)", 1)

/* Bug 118 */
test("eval('function f118(){}'); typeof f118", "function");

finish()
