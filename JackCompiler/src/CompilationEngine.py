from JackTokenizer import JackTokenizer
from VMGenerator import VMGenerator
from symbolTable import JackClass
from symbolTable import Subroutine
convert_binary_op = {'+': 'add','-': 'sub','*': 'call Math.multiply 2','/': 'call Math.divide 2','&': 'and','|': 'or','<': 'lt','>': 'gt','=': 'eq'}
label_count = 0
class CompilationEngine:
    """
    Jack compilation engine
    """
    def __init__(self, tokenizer, output_path): 
        self._indentation = 2
        self.tokenizer = tokenizer
        self.vm_writer = VMGenerator(output_path)

    @staticmethod
    def label():
        "used to distinct labels"
        global label_count
        label = 'L{}'.format(label_count)
        label_count += 1
        return label

    def compile_class(self):
        '''Compile a class block'''
        self.tokenizer.advance()
        jack_class = JackClass(self.tokenizer.advance().value)
        self.tokenizer.advance()
        self.compileClassVarDec(jack_class)
        self.compile_class_subroutines(jack_class)
        self.tokenizer.advance()
        
    def compile_class_subroutines(self, jack_class):
        '''Compile the class subroutines'''
        
        token = self.tokenizer.current_token()
        while token is not None and token.type == 'keyword'\
                and token.value in ['constructor', 'function', 'method']:
            
            # Advance for same reason as in varDec
            subroutine_type = self.tokenizer.advance().value
            # return type
            return_type = self.tokenizer.advance().value
            # name
            name = self.tokenizer.advance().value

            jack_subroutine = Subroutine(
                    name, subroutine_type, return_type, jack_class
                )
            #enter parameter list
            self.tokenizer.advance() 
            self.compile_parameter_list(jack_subroutine)

            #continue to compile body of subroutine
            self.tokenizer.advance()
            self.compile_subroutine_body(jack_subroutine)

            # load the next token to check 
            token = self.tokenizer.current_token()
    
    "compile the var declaritions in a class"
    def compileClassVarDec(self, jack_class):
        token = self.tokenizer.current_token()
        #test if it's var declarition
        while token is not None and token.type == 'keyword' and\
                token.value in ['static', 'field']:
            self.tokenizer.advance()
            is_static = token.value == 'static'
            # get the var type
            var_type = self.tokenizer.advance().value
            more_vars = True
            while more_vars:
                var_name = self.tokenizer.advance().value
                if is_static:
                    jack_class.add_static(var_name, var_type)
                else:
                    jack_class.add_field(var_name, var_type)

                token = self.tokenizer.advance()
                more_vars = token == ('symbol', ',')
            token = self.tokenizer.current_token()

    def compile_parameter_list(self, jack_subroutine):
        '''Compile a parameter list for a subroutine'''
        token = self.tokenizer.current_token()
        # Check if the next token is a valid variable type
        more_vars = token is not None and token.type in ['keyword', 'identifier']
        while more_vars:
            token = self.tokenizer.advance()
            param_type = token.value
            param_name = self.tokenizer.advance().value
            jack_subroutine.add_arg(param_name, param_type)
            token = self.tokenizer.current_token()
            # check more vars exist
            if token == ('symbol', ','):
                self.tokenizer.advance()
                token = self.tokenizer.current_token()
                more_vars = token is not None and token.type in ['keyword', 'identifier']
            else:
                more_vars = False


    def compile_subroutine_body(self, jack_subroutine):
        self.tokenizer.advance()
        self.compile_subroutine_vars(jack_subroutine)
        self.vm_writer.write_function(jack_subroutine)
        #branch different subtine type
        if jack_subroutine.subroutine_type == 'constructor':
            field_count = jack_subroutine.jack_class.nfield
            self.vm_writer.write_push('constant', field_count)
            self.vm_writer.write_call('Memory', 'alloc', 1)
            self.vm_writer.write_pop('pointer', 0)
        elif jack_subroutine.subroutine_type == 'method':
            self.vm_writer.write_push('argument', 0)
            self.vm_writer.write_pop('pointer', 0)
        self.compile_statements(jack_subroutine)
        self.tokenizer.advance()

    def compile_subroutine_vars(self, jack_subroutine):
        token = self.tokenizer.current_token()
        # Check that a variable declarations starts
        while token is not None and token == ('keyword', 'var'):
            self.tokenizer.advance()
            var_type = self.tokenizer.advance().value
            var_name = self.tokenizer.advance().value
            jack_subroutine.add_var(var_name, var_type)
            while self.tokenizer.advance().value == ',':
                # var_name
                var_name = self.tokenizer.advance().value
                jack_subroutine.add_var(var_name, var_type)
            token = self.tokenizer.current_token()
    
    def compile_statements(self, jack_subroutine):
        check_statements = True
        while check_statements:
            token = self.tokenizer.current_token()
            if token == ('keyword', 'if'):
                self.compile_statement_if(jack_subroutine)
            elif token == ('keyword', 'while'):
                self.compile_statement_while(jack_subroutine)
            elif token == ('keyword', 'let'):
                self.compile_statement_let(jack_subroutine)
            elif token == ('keyword', 'do'):
                self.compile_statement_do(jack_subroutine)
            elif token == ('keyword', 'return'):
                self.compile_statement_return(jack_subroutine)
            else:
                check_statements = False
    
    def compile_statement_if(self, jack_subroutine):
        #advance two times for if and (
        self.tokenizer.advance() 
        self.tokenizer.advance() 
        self.compile_expression(jack_subroutine)
        #advance twice for ) and }
        self.tokenizer.advance() 
        self.tokenizer.advance() 
        #get labels that will be used in jump command
        false_label = CompilationEngine.label()
        end_label = CompilationEngine.label()
        self.vm_writer.write_if(false_label)
        # Compile inner statements
        self.compile_statements(jack_subroutine)
        self.vm_writer.write_goto(end_label)
        self.vm_writer.write_label(false_label)
        #advance for }
        self.tokenizer.advance() 
        #test if 'else' exists
        token = self.tokenizer.current_token()
        if token == ('keyword', 'else'):
            self.tokenizer.advance() # else
            self.tokenizer.advance() # {
            # Compile inner statements
            self.compile_statements(jack_subroutine)
            #advance for }
            self.tokenizer.advance() # }
        self.vm_writer.write_label(end_label)


    def compile_statement_while(self, jack_subroutine):
        #advance twice for while and (
        self.tokenizer.advance() 
        self.tokenizer.advance()
        while_label = self.label()
        false_label = self.label()
        self.vm_writer.write_label(while_label)        
        self.compile_expression(jack_subroutine)
        #advance twice for ) and {
        self.tokenizer.advance() 
        self.tokenizer.advance()
        self.vm_writer.write_if(false_label)
        # Compile inner statements
        self.compile_statements(jack_subroutine)
        self.vm_writer.write_goto(while_label)
        self.vm_writer.write_label(false_label)
        self.tokenizer.advance()
    
    def compile_statement_let(self, jack_subroutine):
        self.tokenizer.advance()
        var_name = self.tokenizer.advance().value 
        jack_symbol = jack_subroutine.get_symbol(var_name)
        is_array = self.tokenizer.current_token().value == '['
        if is_array:
            self.tokenizer.advance() # [
            self.compile_expression(jack_subroutine) # Index
            self.tokenizer.advance() # ]
            self.tokenizer.advance() # =
            # Add the base and offset
            self.vm_writer.write_push_symbol(jack_symbol)
            self.vm_writer.write('add')
            self.compile_expression(jack_subroutine) # lexpression 
            self.vm_writer.write_pop('temp', 0) # Store assigned value in temp
            self.vm_writer.write_pop('pointer', 1) # Restore destination
            self.vm_writer.write_push('temp', 0) # Restore assigned value
            self.vm_writer.write_pop('that', 0) # Store in target
        else:
            #advance for '='
            self.tokenizer.advance() 
            self.compile_expression(jack_subroutine) # Rexpression to assign
            self.vm_writer.write_pop_symbol(jack_symbol)

        self.tokenizer.advance() 

    def compile_statement_do(self, jack_subroutine):
        #advance for do
        self.tokenizer.advance() 
        self.compile_term(jack_subroutine) 
        self.vm_writer.write_pop('temp', 0) 
        self.tokenizer.advance()

    def compile_statement_return(self, jack_subroutine):
        self.tokenizer.advance() 
        # Check if an expression is given
        token = self.tokenizer.current_token()
        if token != ('symbol', ';'):
            self.compile_expression(jack_subroutine)
        else:
            self.vm_writer.write_int(0)
        self.vm_writer.write_return()
        self.tokenizer.advance()
    

    def compile_expression_list(self, jack_subroutine):
        '''Compile a subroutine call expression_list'''
        # the number of expresions in the list
        count = 0 
        token = self.tokenizer.current_token()
        while token != ('symbol', ')'):
            if token == ('symbol', ','):
                self.tokenizer.advance()
            count += 1
            self.compile_expression(jack_subroutine)
            token = self.tokenizer.current_token()

        return count


    def compile_expression(self, jack_subroutine):
        self.compile_term(jack_subroutine)
        token = self.tokenizer.current_token()
        while token.value in '+-*/&|<>=':
            binary_op = self.tokenizer.advance().value
            self.compile_term(jack_subroutine)
            self.vm_writer.write(convert_binary_op[binary_op])
            token = self.tokenizer.current_token()

    def compile_term(self, jack_subroutine):
        token = self.tokenizer.advance()
        # special case unioperation
        if token.value in ['-', '~']:
            self.compile_term(jack_subroutine)
            if token.value == '-':
                self.vm_writer.write('neg')
            elif token.value == '~':
                self.vm_writer.write('not')

        # expression enclosed inside ()
        elif token.value == '(':
            self.compile_expression(jack_subroutine)
            self.tokenizer.advance() 
        elif token.type == 'integerConstant':
            self.vm_writer.write_int(token.value)
        elif token.type == 'stringConstant':
            self.vm_writer.write_string(token.value)
        elif token.type == 'keyword':
            if token.value == 'this':
                self.vm_writer.write_push('pointer', 0)
            else:
                self.vm_writer.write_int(0) 
                if token.value == 'true':
                    self.vm_writer.write('not')

        
        #expression is function or variable 
        elif token.type == 'identifier':
            token_value = token.value
            token_var = jack_subroutine.get_symbol(token_value)
            token = self.tokenizer.current_token()
            #term is array
            if token.value == '[': 
                self.tokenizer.advance() 
                self.compile_expression(jack_subroutine)
                self.vm_writer.write_push_symbol(token_var)
                #computer the actual address of array
                self.vm_writer.write('add')
                self.vm_writer.write_pop('pointer', 1)
                self.vm_writer.write_push('that', 0)
                self.tokenizer.advance()
            else:
                # the function call case
                func_name = token_value
                func_class = jack_subroutine.jack_class.name
                default_call = True
                arg_count = 0
                #this case implies a method call
                if token.value == '.':
                    default_call = False
                    self.tokenizer.advance() # .
                    # get the obj name
                    func_obj = jack_subroutine.get_symbol(token_value)
                    func_name = self.tokenizer.advance().value
                    if func_obj:
                        func_class = token_var.type 
                        # Add 'this' to args
                        arg_count = 1 
                        # push "this"
                        self.vm_writer.write_push_symbol(token_var) 
                    else:
                        func_class = token_value
                    token = self.tokenizer.current_token()

                # this case implies a function call
                if token.value == '(':
                    if default_call:
                        # Default call is a method one, push this
                        arg_count = 1
                        self.vm_writer.write_push('pointer', 0)

                    self.tokenizer.advance() # (
                    arg_count += self.compile_expression_list(jack_subroutine)
                    self.vm_writer.write_call(func_class, func_name, arg_count)
                    self.tokenizer.advance() # )
                # If a variable instead
                elif token_var:
                    self.vm_writer.write_push_symbol(token_var)