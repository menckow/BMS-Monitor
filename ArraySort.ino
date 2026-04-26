// Array sortieren
void bsort(name_t arr[], uint16_t len, bool caseSensitive = true)
{
  uint16_t swapCnt;
  name_t tmp, s1, s2;
  do
  {
    swapCnt = 0;
    for (uint16_t i = 1; i < len; i++)
    {
      strcpy(s1, arr[i - 1]);
      strcpy(s2, arr[i]);
      if (!caseSensitive)                                            // Groß/Kleinschreibung ignorieren ?
      {
        strToLower(s1);
        strToLower(s2);
      }
      if (strcmp(s1, s2) > 0)                                        // tauschen
      {
        strcpy(tmp, arr[i]);
        strcpy(arr[i], arr[i - 1]);
        strcpy(arr[i - 1], tmp);
        swapCnt++;
      }
    }
    len--;
  } while (swapCnt != 0);
}

//----------------------------------------------------
void strToLower(name_t &str)                                       // String in Kleinbuchstabe
{
  for (byte i = 0; i < strlen(str); i++)
  {
    str[i] = tolower(str[i]);
  }
}
//----------------------------------------------------
