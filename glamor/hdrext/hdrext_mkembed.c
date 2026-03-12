/**
  similar to xxd: it generates c include files for shaders in "readable" format with appended null
**/

#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>

int main(int argc,char *argv[]){
    if (argc != 4){
      printf("usage:%s variable-name  inputfile output-without-extension\n",argv[0]);
    }


  FILE *fin = fopen(argv[2],"r");
  FILE *fout = fopen(argv[3],"w");

  char *variable_name = strdup(argv[1]);

  for(size_t i=0;variable_name[i] != '\0';++i){
      if (variable_name[i] == '-' || variable_name[i] == '.'){
          variable_name[i]  = '_';
      }
  }
  //fprintf(fout,"#include <stdint.h>\n");
  fprintf(fout,"static const unsigned char %s[] = {\n",variable_name);
  free(variable_name);

  fprintf(fout,"    \"");
 for(;;){
    int r = fgetc(fin);
    if (r == EOF){
        break;
    }

    uint8_t c = (r & 0xff);

    if (c == '\n'){
        fputs("\\n\"\n    \"",fout);
    } else if (c == '\t'){
        fputs("\\t",fout);
    }else if (c == '\r'){
        fputs("\\r",fout);
    } else if ( c == '"'){
        fputs("\\\"",fout);
    } else {
        fputc(c,fout);
    }
 }

 fputs("\"};\n",fout);

fclose(fout);
fclose(fin);
}




