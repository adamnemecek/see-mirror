
describe("Exercises the File module");

test("typeof File", "object");
test("typeof File.In", "object");
test("typeof File.Out", "object");
test("typeof File.Err", "object");

test("File.prototype.eof()", undefined);
test('File.Out.write("Hello! on stdout\\n")', undefined);
test('File.Out.write("\\u2020")', Exception(RangeError));
test('File.Err.write("Hello! on stderr\\n")', undefined);

test('new File("this-file-should-not-exist", "r")', 
    new ExceptionInstance(File.FileError));

var f, g;
test('f = new File("t-File.out", "w")', NO_EXCEPTION);
test('f.write("Testing\\n")', undefined);
test('f.flush()', undefined);
test('f.eof()', false);
test('f.close()', undefined);
test('f.eof()', undefined);

test('g = new File("t-File.out", "r")', NO_EXCEPTION);
test('g.eof()', false);
test('g.read(2)', "Te");
test('g.eof()', false);
test('g.read()', "sting\n");
test('g.eof()', true);
test('g.close()', undefined);
test('g.eof()', undefined);

