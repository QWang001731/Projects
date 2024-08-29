import sys
import os
import JackTokenizer
import CompilationEngine

def compile_file(file_path):		
	with open(file_path, 'r') as ifile:
		file_name = os.path.basename(file_path)
		file_path_no_ext, _ = os.path.splitext(file_path)
		file_name_no_ext, _ = os.path.splitext(file_name)

		ofile_path = file_path_no_ext+'.vm'
		with open(ofile_path, 'w') as ofile:
			tokenizer = JackTokenizer.JackTokenizer(ifile.read())
			compiler = CompilationEngine.CompilationEngine(tokenizer, ofile)
			compiler.compile_class()

def compile_dir(dir_path):
	'''Compile all Jack files in a directory'''
	for file in os.listdir(dir_path):
		file_path = os.path.join(dir_path, file)
		_, file_ext = os.path.splitext(file_path)
		if os.path.isfile(file_path) and file_ext.lower() =='.jack':
			compile_file(file_path)


def main():
	if os.path.isdir(sys.argv[1]):
		compile_dir(sys.argv[1])
	elif os.path.isfile(sys.argv[1]):
		compile_file(sys.argv[1])
	else:
		print("input is invalid")
		sys.exit(1)


if __name__ == "__main__":
    main()

