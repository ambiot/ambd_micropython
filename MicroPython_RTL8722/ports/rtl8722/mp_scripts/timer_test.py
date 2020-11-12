from umachine import Timer
t = Timer(0)

def fun():
	print("timer fired")

fun()
time.sleep_ms(500)
fun()
time.sleep_ms(500)
fun()