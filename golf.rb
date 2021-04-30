# this script removes columns from 1L_a HW program and checks if it still works
# successfully removes count=19 columns
require 'timeout'

def run
  begin
    result = Timeout::timeout(0.2) { `1l_a105.exe tmp` }
    return result == "Hello, World!"
  rescue Timeout::Error
    `taskkill /f /im 1l_a105.exe`
	return false
  end
end

#prog = ["123\n","abc\n"]
#prog1 = prog.map{ |line| line1 = line.dup; line1.slice!(0,2); line1 }
#print prog1
#exit

prog = File.readlines("hw.golf") # hangs on hw, so using hw.golf
count = 0
i = 0
while i < prog[0].length-1  # -1: \n
	prog1 = prog.map{ |line| line1 = line.dup; line1.slice!(i); line1 }
	File.write("tmp", prog1.join)
	if run
		prog = prog1
		count += 1
	else
		i += 1
	end
	#STDERR.puts "i = #{i}, count = #{count}"
end
puts prog.join
puts "count = #{count}"
