
describe("Exercises every production in the grammar");

/* FutureReservedWord */
test("var abstract", Exception(SyntaxError));
test("var boolean", Exception(SyntaxError));
test("var byte", Exception(SyntaxError));
test("var char", Exception(SyntaxError));
test("var class", Exception(SyntaxError));
test("var const", Exception(SyntaxError));
test("var debugger", Exception(SyntaxError));
test("var enum", Exception(SyntaxError));
test("var export", Exception(SyntaxError));
test("var extends", Exception(SyntaxError));
test("var final", Exception(SyntaxError));
test("var float", Exception(SyntaxError));
test("var goto", Exception(SyntaxError));
test("var implements", Exception(SyntaxError));
test("var int", Exception(SyntaxError));
test("var interface", Exception(SyntaxError));
test("var long", Exception(SyntaxError));
test("var native", Exception(SyntaxError));
test("var package", Exception(SyntaxError));
test("var private", Exception(SyntaxError));
test("var protected", Exception(SyntaxError));
test("var short", Exception(SyntaxError));
test("var static", Exception(SyntaxError));
test("var super", Exception(SyntaxError));
test("var synchronized", Exception(SyntaxError));
test("var throws", Exception(SyntaxError));
test("var transient", Exception(SyntaxError));
test("var volatile", Exception(SyntaxError));
test("var double", Exception(SyntaxError));
test("var import", Exception(SyntaxError));
test("var public", Exception(SyntaxError));

test("1 + +1", 2);
test("1 - +1", 0);
test("1 + -1", 0);
test("1 + +1", 2);

test("this", this);
test("null", null);
test("undefined", undefined);
test("'\\u0041'", "A");
test('"\\x1b\\x5bm"', "[m");
test('"\\33\\133m"', "[m");
ident = "ident"; test("ident", "ident");
test("[1,2,3].length", 3);
test("({'foo':5}.foo)", 5);
test("((((3))))", 3);

function constr() {
	return constr;
}
constr.prototype = Object.prototype

test("new new new constr()", constr);
test("(1,2,3)", 3);
test("i = 3; i++", 3); test("i", 4);
test("i = 3; ++i", 4); test("i", 4);
test("i = 3; i--", 3); test("i", 2);
test("i = 3; --i", 2); test("i", 2);
test("i = 3; i ++ ++", Exception(SyntaxError));
test("i = 3; --i++", Exception(ReferenceError));
test("i = 3; ++i--", Exception(ReferenceError));

test("!true", false);
test("~0", -1);
test("void 'hi'", undefined);
test("i = 3; delete i", true); test("i", Exception(ReferenceError));

test("3 * 6 + 1", 19);
test("1 + 3 * 6", 19);
test("17 % 11 * 5", 30);
test("30 / 3 / 5", 2);
test("30 / 3 * 5", 50);

test("1 - 1 - 1", -1);

test("i=3;j=5; i*=j+=i", 24);

/* instanceof's rhs must be an object */
test("1 instanceof 1", Exception(TypeError));
test("null instanceof null", Exception(TypeError));

/* Only function objects should support HasInstance: */
test("1 instanceof Number.prototype", Exception(TypeError));
test("new Number(1) instanceof Number.prototype", Exception(TypeError));	


/* Test the instanceof keyword and the new operator applied to functions. */
function Employee(age, name) {
	this.age = age;
	this.name = name;
}
Employee.prototype = new Object()
Employee.prototype.constructor = Employee
Employee.prototype.toString = function() {
	return "Name: " + this.name + ", age: " + this.age;
}
Employee.prototype.retireable = function() { return this.age > 55; }

function Manager(age, name, group) {
	this.age = age;
	this.name = name;
	this.group = group;
}
Manager.prototype = new Employee();
Manager.prototype.toString = function() {
	return "Name: " + this.name + ", age: " + this.age
	       + ", group: " + this.group;
}

e = new Employee(24, "Tony");
m = new Manager(62, "Paul", "Finance");
test("m.retireable()", true);
test("m instanceof Employee", true);
test("e instanceof Manager", false);

test("{true;}", true);
test(";", undefined);
test("label:;", undefined);
test("label:label2:;", undefined);
test("label:label2:label3:;", undefined);
test("label:label2:label3:break label;", undefined);
test("{}", undefined);
test("i=0; do { i++; } while(i<10); i", 10);
test("i=0; while (i<10) { i++; }; i", 10);
test("for (i = 0; i < 10; i++); i", 10);
test("i=0; for (; i < 10; i++); i", 10);
test("i=0; for (; i < 10; ) i++; i", 10);
test("i=0; for (; ;i++) if (i==10) break; i", 10);
test("a=[1,2,3,4]; c=0; for (var v in a) c+=a[v]; c", 10);
test("delete t; t", Exception(ReferenceError));
test("{var t;} t", undefined);
test("continue", Exception(SyntaxError));
test("return", Exception(SyntaxError));
test("break", Exception(SyntaxError));
test("x = 0; outer: for (;;) { for (;;) break outer; x++; }; x", 0);
test("x = 0; for (i = 0; i < 3; i++) { continue; x++; } x", 0);
test("x = 0; it:for (i = 0; i < 3; i++) { for (;;) continue it; x++; } x", 0);
test("c = 9; o = { a:'a', b: { c: 'c' }, c:7 }; with (o.b) x = c; x", 'c');
test("x = ''; for (i = 0; i < 8; i++) switch (i) {" +
     "case 0: x+='a'; case 1: x+='b'; break;" +
     "case 2: x+='c'; break; case 3: x+='d'; default: x+='e';" +
     "case 4: x+='f'; break; case 5: x+='g'; case 6: x+='h';}; x",
     "abbcdeffghhef");
test("foo:bar:baz:;", undefined);
var obj = {};
test("throw obj", Exception(obj));
test("x=0;try{throw {a:1}} catch(e){x=e.a};x", 1);
test("x=y=0;try{" +
     " try { throw {a:1} } finally {x=2}; " +
     "} catch(e) {y=e.a}; x+y", 3);
test("x=y=0; try{throw {a:2};y=1;} catch(e){x=e.a;y=-7;} finally{y=3}; x+y", 5);
compat("js15");
test("var x='pass';a:{b:break a;x='fail';};x", 'pass');
test("if (0) function foo(){}", undefined);


finish()
