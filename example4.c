extern void someFunction(int i);

__attribute__((used)) void callSomeFunction (int i)
{
  someFunction(i);
}
