%option noyywrap

%{
int chars = 0;
int words = 0;
int lines = 0;
%}

%%
abc/de  {yyless(3); printf("we got abd");}
de	{printf("we got de !!!");}
%%

int main(){
 yylex();
 return 0;
}
                                                                               
