
describe("Global object tests")

var Global = this;

/* 15.1 */
test("new Global()", Exception(TypeError));
test("Global()", Exception(TypeError));
test("Global['NaN']", 0.0/0.0)
test("Global['Infinity']", 1.0/0.0)
test("Global['undefined']", undefined)
/* 15.1.2.2 */
test("parseInt('123')", 123)
test("parseInt('-123')", -123)
test("parseInt('0x10')", 16)
test("parseInt('0x10', 0)", 16)
test("parseInt('0x10', undefined)", 16)
test("parseInt('0x10', 16)", 16)
test("parseInt('0x10', 15)", 0)
test("parseInt('0X10')", 16)
test("parseInt('10', 16)", 16)
test("parseInt('10', 19)", 19)
test("parseInt('+0x10')", 16)
test("parseInt('-0x10')", -16)
test("parseInt('-+123')", NaN)
test("parseInt('+-123')", NaN)
test("parseInt('-')", NaN)
test("parseInt('- 123')", NaN)
test("parseInt('  123')", 123)
test("parseInt('  -123')", -123)
test("parseInt('abz',36)", 10*36*36+11*36+35)
test("parseInt('123',3)", 3+2)
/* 15.1.2.3 */
test("parseFloat('123.456')", 123.456)
test("parseFloat('-123.456')", -123.456)
test("parseFloat('-123.456e9')", -123.456e9)
test("parseFloat('+123.456e9')", +123.456e9)
test("parseFloat('+-1.5')", NaN)
test("parseFloat('-+1.5')", NaN)
test("parseFloat('  +1.5')", +1.5)
test("parseFloat('  -1.5')", -1.5)
test("parseFloat('  -')", NaN)
test("parseFloat('')", NaN)
test("parseFloat('.')", NaN)
test("parseFloat('.0')", 0)
test("parseFloat('0.')", 0)
test("parseFloat('-0.')", -0)
test("parseFloat('+0.')", +0)
test("parseFloat('-.0')", -0)
test("parseFloat('+.0')", +0)
test("parseFloat('1.5z')", 1.5)
test("parseFloat('1.5e-1')", 1.5e-1)
test("parseFloat('1.5e1')", 1.5e1)
test("parseFloat('1.5e+1')", 1.5e+1)
/* 15.1.2.4 */
test("isNaN('')", false)	// ToNumber('') -> 0
test("isNaN('garbage')", true)
test("isNaN('0 ')", false)
test("isNaN(' 0')", false)
test("isNaN('0')", false)
test("isNaN(NaN)", true)
test("isNaN(0)", false)
test("isNaN(Infinity)", false)
test("isNaN(-Infinity)", false)
test("isNaN(undefined)", true)	// ToNumber(undefined) -> NaN
test("isNaN()", true)		// ToNumber(undefined) -> NaN
/* 15.1.2.5 */
test("isFinite(NaN)", false)
test("isFinite(0)", true)
test("isFinite(Infinity)", false)
test("isFinite(-Infinity)", false)
test("isFinite(undefined)", false)
test("isFinite()", false)	// ToNumber(undefined) -> NaN
/* 15.1.3.1 */
test("decodeURI('')", '')
test("decodeURI('abc')", 'abc')
test("decodeURI('%')", Exception(URIError))
test("decodeURI('%6')", Exception(URIError))
test("decodeURI('%g0')", Exception(URIError))
test("decodeURI('%0g')", Exception(URIError))
test("decodeURI('%61')", 'a')
test("decodeURI('%0a')", '\x0a')
test("decodeURI('%0A')", '\x0a')
/* Test the group assertions */
var unescaped='abcdefghijklmnopqrstuvwxyz' +
	      'ABCDEFGHIJKLMNOPQRSTUVWXYZ' +
	      '0123456789' +
	      '-_.!~*\'()';
var reserved =';/?:@&=+$,';
/* other = all - {unescaped + reserved + '#'} */
var other = '\x00\x01\x02\x03\x04\x05\x06\x07' +
	    '\x08\x09\x0a\x0b\x0c\x0d\x0e\x0f' +
            '\x10\x11\x12\x13\x14\x15\x16\x17' +
	    '\x18\x19\x1a\x1b\x1c\x1d\x1e\x1f' +
            '\x20'+ '\x22'   +  '\x25'         +
	                    '\x3c'+ '\x3e'     +
	                '\x5b\x5c\x5d\x5e'     +
            '\x60'                             +
	                '\x7b\x7c\x7d'+ '\x7f';
function hex(s) {   // Helper function
    var res = '';
    for (var i = 0; i < s.length; i++) {
	var c = s.charCodeAt(i);
	if (c < 16) res = res + "%0" + c.toString(16).toUpperCase()
	else res = res + "%" + c.toString(16).toUpperCase()
    }
    return res;
}
test("hex('\\x05abc')", "%05%61%62%63")	    // sanity check the helper function
/* 15.1.3.1 */
test("decodeURI(hex(reserved))", hex(reserved));
test("decodeURI(hex('#'))", hex('#'));
test("decodeURI(hex(unescaped))", unescaped)
test("decodeURI(hex(other))", other)
/* 15.1.3.2 */
test("decodeURIComponent(hex(reserved))", reserved)
test("decodeURIComponent(hex('#'))", '#')
test("decodeURIComponent(hex(unescaped))", unescaped)
test("decodeURIComponent(hex(other))", other)
/* 15.1.3.3 */
test("encodeURI(reserved)", reserved)
test("encodeURI('#')", '#')
test("encodeURI(unescaped)", unescaped)
test("encodeURI(other)", hex(other))
/* 15.1.3.4 */
test("encodeURIComponent(reserved)", hex(reserved))
test("encodeURIComponent('#')", hex('#'))
test("encodeURIComponent(unescaped)", unescaped)
test("encodeURIComponent(other)", hex(other))

finish()
