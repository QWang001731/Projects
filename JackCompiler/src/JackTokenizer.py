import re
import sys
from collections import namedtuple
COMMENT = "(//.*)|(/\*([^*]|[\r\n]|(\*+([^*/]|[\r\n])))*\*+/)"
EMPTY_TEXT_PATTERN = re.compile("\s*")
KEY_WORD_PATTERN = re.compile("^\s*("
                              "class|constructor|function|method|static|field"
                              "|var|int|char|boolean|void|true|false|null|this|"
                              "let|do|if|else|while|return)\s*")
RE_SYMBOL = '\{|\}|\(|\)|\[|\]|\.|,|;|\+|-|\*|/|&|\||\<|\>|=|~'
RE_STRING = '"[^"]*"'
SYMBOL_PATTERN = re.compile("^\s*([{}()\[\].,;+\-*/&|<>=~])\s*")
DIGIT_PATTERN = re.compile("^\s*(\d+)\s*")
STRING_PATTERN = re.compile("^\s*\"(.*)\"\s*")
IDENTIFIER_PATTERN = re.compile("^\s*([a-zA-Z_][a-zA-Z1-9_]*)\s*")
Token = namedtuple('Token',('type', 'value'))

class JackTokenizer:
    KEYWORD = 0
    SYMBOL = 1
    INT_CONST = 2
    STRING_CONST = 3
    IDENTIFIER = 4
    SPLIT = '(' + '|'.join(expr for expr in [RE_SYMBOL,RE_STRING]) + ')|\s+'
    '''A tokenizer for the Jack programming language'''
    # The regular expressions for lexical elements in Jack
    RE_INTEGER ='\d+'
    RE_STRING = '"[^"]*"'
    RE_IDENTIFIER = '[A-z_][A-z_\d]*'
    RE_SYMBOL = '\{|\}|\(|\)|\[|\]|\.|,|;|\+|-|\*|/|&|\||\<|\>|=|~'
    RE_KEYWORD = '|'.join(keyword for keyword in
		[
			'class','method','constructor','function','field','static','var',
			'int','char','boolean','void','true','false','null','this','let',
			'do','if','else','while','return'
		])

    TYPES = [(RE_KEYWORD, 'keyword'),(RE_SYMBOL, 'symbol'),(RE_INTEGER, 'integerConstant'),
	(RE_STRING, 'stringConstant'),(RE_IDENTIFIER, 'identifier')]

    def __init__(self, input_file_path):
        self._clear_all_comments(input_file_path)
        self.tokens = self.tokenize()


    def _clear_all_comments(self, input_file_path):
        self.code = re.sub(COMMENT, "", input_file_path)
    
    def advance(self):
        return self.tokens.pop(0) if self.tokens else None
    
    def current_token(self):
        return self.tokens[0] if self.tokens else None
    
    def tokenize(self):
        "tokenize the code for further process"
        split_code = re.split(self.SPLIT, self.code)
        tokens = []
        for lex in split_code:
            # not a tokens continue
            if lex is None or re.match('^\s*$', lex):
                continue
            # other possible  types
            for expr, lex_type in self.TYPES:
                if re.match(expr, lex):
                    tokens.append(Token(lex_type, lex))
                    break
            else:
                print('Error: invalid jack language', lex)
                sys.exit(1)
        return tokens
