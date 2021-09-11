void make_instanced_key(char *out, const char *key, int instanceId) {
  out[0] = key[0];
  out[1] = ':';
  out[2] = ((instanceId / 10) % 10) + '0';
  out[3] = (instanceId % 10) + '0';
  out[4] = 0;
}
