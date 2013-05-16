
describe("Exercises the standard's regular expression examples");

test("String(/a|ab/.exec('abc'))", "a");
test("String(/((a)|(ab))((c)|(bc))/.exec('abc'))", "abc,a,a,,bc,,bc");
test("String(/a[a-z]{2,4}/.exec('abcdefghi'))", "abcde");
test("String(/a[a-z]{2,4}?/.exec('abcdefghi'))", "abc");
test("String(/(aa|aabaac|ba|b|c)*/.exec('aabaac'))", "aaba,ba");
test("'aaaaaaaaaa,aaaaaaaaaaaaaaa'.replace(/^(a+)\\1*,\\1+$/,'$1')", "aaaaa");
test("String(/(z)((a+)?(b+)?(c))*/.exec('zaacbbbcac'))", "zaacbbbcac,z,ac,a,,c")
test("String(/(a*)*/.exec('b'))",",");
test("String(/(a*)b\\1+/.exec('baaaac'))","b,");
test("String(/(?=(a+))/.exec('baaabac'))",",aaa");
test("String(/(?=(a+))a*b\\1/.exec('baaabac'))", "aba,a");
test("String(/(.*?)a(?!(a+)b\\2c)\\2(.*)/.exec('baaabaac'))", 
	"baaabaac,ba,,abaac");

test("'$1,$2'.replace(/(\\$(\\d))/g, '$$1-$1$2')", "$1-$11,$1-$22");
test("String('ab'.split(/a*?/))", "a,b");
test("String('ab'.split(/a*/))", ",b");

finish()
