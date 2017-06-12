#ifndef Student_H
#define Student_H

/******************************************************************************
 * Includes
 */

#include "render.h"
#include "fragment.h"
#include "vector_types.h"
#include "bmp.h"

#ifdef __cplusplus
extern "C" {
#endif

/******************************************************************************
 * renderer
 */

/* Typ/ID rendereru */
extern const int STUDENT_RENDERER;

/* Nazev textury pro nacteni (nastavovan z main.c) */
char * TEXTURE_FILENAME;

/* Jadro rendereru */
typedef struct S_StudentRenderer {

    S_Renderer  base;
    
    S_RGBA *texture;
    
} S_StudentRenderer;


/* Funkce vytvori renderer a nainicializuje jej */
S_Renderer * studrenCreate();

void studrenClearBuffers(S_Renderer *pRenderer); 

/* Funkce korektne zrusi renderer a uvolni pamet */
void studrenRelease(S_Renderer **ppRenderer);

/* Nova fce pro rasterizaci trojuhelniku s podporou texturovani */
void studrenDrawTriangle(S_Renderer *pRenderer,
                         S_Coords *v1, S_Coords *v2, S_Coords *v3,
                         S_Coords *n1, S_Coords *n2, S_Coords *n3,
                         int x1, int y1,
                         int x2, int y2,
                         int x3, int y3,
                         S_Coords *t1, S_Coords *t2, S_Coords *t3,
                         double h1, double h2, double h3
                         );

/* Vykresli i-ty trojuhelnik n-teho klicoveho snimku modelu
 * pomoci nove fce studrenDrawTriangle()
 * Pred vykreslenim aplikuje na vrcholy a normaly trojuhelniku
 * aktualne nastavene transformacni matice!
 * i - index trojuhelniku
 * n - index klicoveho snimku (float pro pozdejsi interpolaci mezi snímky) */
void studrenProjectTriangle(S_Renderer *pRenderer, S_Model *pModel, int i, float n);

/* Vraci hodnotu v aktualne nastavene texture na zadanych
* texturovacich souradnicich u, v
* Pro urceni hodnoty pouziva bilinearni interpolaci
* u, v - texturovaci souradnice v intervalu 0..1, ktery odpovida sirce/vysce textury */
S_RGBA studrenTextureValue( S_StudentRenderer * pRenderer, double u, double v );

/* Upravena funkce pro vyrenderovani cele sceny, tj. vykresleni modelu*/
void renderStudentScene( S_Renderer *pRenderer, S_Model *pModel );

/* Callback funkce volana pri tiknuti casovace 
 * ticks - pocet milisekund od inicializace */
void onTimer( int ticks );

/* Funkce pro vyrenderovani modelu slozeneho z trojuhelniku (viz main.c) 
 * n - index klicoveho snimku (float pro pozdejsi interpolaci mezi snimky) */
void renderModel(S_Renderer *pRenderer, S_Model *pModel, float n);


#ifdef __cplusplus
}
#endif

#endif /* Student_H */

/*****************************************************************************/
/*****************************************************************************/
