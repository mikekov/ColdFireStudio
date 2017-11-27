
print("/* generated file: do not edit */")
print("")
print("cf::uint8 font5x7[][7]=")
print("{")

with open("C:\\Apps\\FontEd\\test.bdf", encoding='Windows-1252') as f:
	f.readline()
	state = 0
	for l in f.readlines():
		try:
			line = l.rstrip()
			if state == 1:
				if line == "ENDCHAR":
					state = 0
					print("},",)
				else:
					print("0x" + line + ", ", end= '')

			elif line == "BITMAP":
				state = 1
				print(" { ", end= '')

		except UnicodeEncodeError:
			state = 0

print("};")
