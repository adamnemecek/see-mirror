
describe("Exercise exception handling");

/* Ensure normal flow of control is unhindered */
test("var c=1; try{       } catch(e){c=2}              c", 1);
test("var c=1; try{throw 0} catch(e){c=2}              c", 2);
test("var c=1; try{       }               finally{c=3} c", 3);
test("var c=1; try{throw 0}               finally{c=3} c", Exception(0));
test("var c=1; try{       } catch(e){c=2} finally{c=3} c", 3);
test("var c=1; try{throw 0} catch(e){c=2} finally{c=3} c", 3);

/* Ensure the 'e' variable is set right */
test("var c=1; try{       } catch(e){c=e}              c", 1);
test("var c=1; try{throw 0} catch(e){c=e}              c", 0);
test("var c=1; try{       } catch(e){c=e} finally{c=3} c", 3);
test("var c=1; try{throw 0} catch(e){c=e} finally{c=3} c", 3);

/* Ensure the 'e' variable is private to the catch function */
test("var e=4,c=1; try{       } catch(e){c=e}              e", 4);
test("var e=4,c=1; try{throw 0} catch(e){c=e}              e", 4);
test("var e=4,c=1; try{       } catch(e){c=e} finally{c=3} e", 4);
test("var e=4,c=1; try{throw 0} catch(e){c=e} finally{c=3} e", 4);

function testf(text, expected) {
    test("(function (){"+text+";})()", expected);
}
testf("return 1", 1); /* test the testf function */

/* Test for various flow control difference */
testf("try{return 1} catch(e){return 2};                   return 4", 1);
testf("try{throw  0} catch(e){return 2};                   return 4", 2);
testf("try{return 1} catch(e){return 2} finally{return 3}; return 4", 3);
testf("try{throw  0} catch(e){return 2} finally{return 3}; return 4", 3);
testf("try{return 1} catch(e){throw  2};                   return 4", 1);
testf("try{throw  0} catch(e){throw  2};                   return 4", 
							    Exception(2));
testf("try{return 1} catch(e){throw  2} finally{throw  3}; return 4",
							    Exception(3));
testf("try{throw  0} catch(e){throw  2} finally{throw  3}; return 4",
							    Exception(3));

finish()
