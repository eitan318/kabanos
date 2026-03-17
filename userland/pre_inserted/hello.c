int main() {
  const char *msg = "Hello from MyOS!\n";
  write(1, msg, 18);
  return 0;
}
