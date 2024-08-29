from collections import namedtuple
convert = {'static': 'static', 'field': 'this', 'arg': 'argument', 'var': 'local'}
JackSymbol = namedtuple('Symbol', ['kind', 'type', 'id'])
class VMGenerator:
    def __init__(self, ostream):
        self.ostream = ostream
        self.nbalel = 0

    def write_if(self, label):
        self.ostream.write("not\n")
        self.ostream.write("if-goto {}\n".format(label))
    
    def write_goto(self, label):
        self.ostream.write("goto {}\n".format(label))
    
    def write_label(self, label):
        self.ostream.write('label {}\n'.format(label))
    
    def write_function(self, subroutine,):
        self.ostream.write("function {0}.{1} {2}\n".format(subroutine.jack_class.name, subroutine.name, subroutine.var_symbols))
    
    def write_return(self):
        self.ostream.write("return\n")
    
    def write_call(self, class_name, func_name, nargs):
        self.ostream.write("call {0}.{1} {2}\n".format(class_name, func_name, nargs))
    
    def write_pop(self, seg, offset):
        self.ostream.write("pop {0} {1}\n".format(seg, offset))
    
    def write_pop_symbol(self, symbol):
        kind = symbol.kind
        offset = symbol.id
        segment = convert[kind]
        self.write_pop(segment, offset)
 
    def write_push(self, seg, offset):    
        self.ostream.write("push {0} {1}\n".format(seg, offset))
    
    def write_push_symbol(self, symbol):
        
        self.ostream.write("push {} {} \n".format(convert[symbol.kind], symbol.id))
    
    def write(self, something):
        self.ostream.write("{}\n".format(something))
    
    def write_int(self, n):
        self.ostream.write("push constant {}\n".format(n))
    
    def write_string(self, s):
        "need to remove the double quotes"
        s1 = s[1:-1]
        self.write_int(len(s1))
        self.write_call("String", "new", 1)
        for c in s1:
            self.write_int(ord(c))
            self.write_call('String', 'appendChar', 2)
    




    


  

    