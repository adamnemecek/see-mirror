
describe("Function object tests")

test("Function()(1,2,3)", undefined)
test("(new Function())(1,2,3)", undefined)
test("(Function('x','return x*x'))(7)", 49)
test("(Function('x','y','return x+y'))(1,7)", 8)
test("(Function(' x,y ','return x+y'))(1,7)", 8)
test("(Function(' x,y ',undefined))(1,7)", undefined)

test("Function.length", 1)

test("getClass(Function.prototype)", "Function")
test("Function.prototype(1,2,3,4)", undefined)
test("Object.prototype.isPrototypeOf(Function.prototype)", true);
test("Function.prototype.constructor === Function", true);
test("Function.prototype.toString.apply({})", Exception(TypeError))
test("typeof Function().toString()", "string")

function quadratic(a, b, c) { this.a = a; this.b = b; this.c = c; }
quadratic.prototype = {
    calc: function (x) { return this.a * x * x + this.b * x + this.c; }
}
test("quadratic.prototype.calc.apply({a:1,b:0,c:0}, [9])", 81)
test("Function.prototype.apply.length", 2)
test("quadratic.prototype.calc.call({a:1,b:0,c:0}, 9)", 81)
test("Function('return this.Function').call(null) === Function", true)
test("Function.prototype.call.length", 1)

/* more TBD */
