/**
 */

#include "student.h"
#include "transform.h"
#include "fragment.h"

#include <memory.h>
#include <math.h>


/*****************************************************************************
 * Globalni promenne a konstanty
 */
int TEXTURE_SIZE = 256;
int change = 0;
float frame = 0;
int time_z = 0;
int time_y;

/* Typ/ID rendereru */
const int STUDENT_RENDERER = 1;


/*****************************************************************************
 * Funkce vytvori renderer a nainicializuje jej
 */

S_Renderer * studrenCreate()
{
    S_StudentRenderer * renderer = (S_StudentRenderer *)malloc(sizeof(S_StudentRenderer));
    IZG_CHECK(renderer, "Cannot allocate enough memory");

    /* inicializace default rendereru */
    renderer->base.type = STUDENT_RENDERER;
    renInit(&renderer->base);

    /* nastaveni ukazatelu na upravene funkce */
    renderer->base.releaseFunc = studrenRelease;
    renderer->base.projectTriangleFunc = studrenProjectTriangle;

    renderer->base.clearBuffersFunc = studrenClearBuffers;

    /* inicializace nove pridanych casti */
    renderer->texture = loadBitmap( TEXTURE_FILENAME, &TEXTURE_SIZE, &TEXTURE_SIZE );
   

    return (S_Renderer *)renderer;
}

void studrenClearBuffers(S_Renderer *pRenderer)
{
    int i, count;

    IZG_ASSERT(pRenderer);

    /* vsechny pixely cerne, tedy 0 */
    memset(pRenderer->frame_buffer, 0, pRenderer->frame_w * pRenderer->frame_h * sizeof(S_RGBA));

    /* vymazeme depth buffer */
    count = pRenderer->frame_w * pRenderer->frame_h;
    for( i = 0; i < count; ++i )
    {
        *(pRenderer->depth_buffer + i) = pRenderer->max_depth;
    }
}

/*****************************************************************************
 * Funkce korektne zrusi renderer a uvolni pamet
 */

void studrenRelease(S_Renderer **ppRenderer)
{
    S_StudentRenderer * renderer;

    if( ppRenderer && *ppRenderer )
    {
        /* ukazatel na studentsky renderer */
        renderer = (S_StudentRenderer *)(*ppRenderer);

        /* pripadne uvolneni pameti */
        free(renderer->texture);
        /* uvolneni pameti frame bufferu */
        if( (*ppRenderer)->frame_buffer )
        {
            free((*ppRenderer)->frame_buffer);
        }

        /* uvolneni pameti depth bufferu */
        if( (*ppRenderer)->depth_buffer )
        {
            free((*ppRenderer)->depth_buffer);
        }

        free(*ppRenderer);
    }
}

/******************************************************************************
 * Nova fce pro rasterizaci trojuhelniku s podporou texturovani
 * v1, v2, v3 - ukazatele na vrcholy trojuhelniku ve 3D pred projekci
 * n1, n2, n3 - ukazatele na normaly ve vrcholech ve 3D pred projekci
 * x1, y1, ... - vrcholy trojuhelniku po projekci do roviny obrazovky
 */

void studrenDrawTriangle(S_Renderer *pRenderer,
                         S_Coords *v1, S_Coords *v2, S_Coords *v3,
                         S_Coords *n1, S_Coords *n2, S_Coords *n3,
                         int x1, int y1,
                         int x2, int y2,
                         int x3, int y3,
                         S_Coords *t1, S_Coords *t2, S_Coords *t3,
                         double h1, double h2, double h3
                         )
{
    /* zaklad fce zkopirujte z render.c */

    int         minx, miny, maxx, maxy;
    int         a1, a2, a3, b1, b2, b3, c1, c2, c3;
    int         s1, s2, s3;
    int         x, y, e1, e2, e3;
    double      alpha, beta, gamma, w1, w2, w3, z, tex_x, tex_y;
    S_RGBA      col1, col2, col3, color;

    IZG_ASSERT(pRenderer && v1 && v2 && v3 && n1 && n2 && n3);

    /* vypocet barev ve vrcholech */
    col1 = pRenderer->calcReflectanceFunc(pRenderer, v1, n1);
    col2 = pRenderer->calcReflectanceFunc(pRenderer, v2, n2);
    col3 = pRenderer->calcReflectanceFunc(pRenderer, v3, n3);

    /* obalka trojuhleniku */
    minx = MIN(x1, MIN(x2, x3));
    maxx = MAX(x1, MAX(x2, x3));
    miny = MIN(y1, MIN(y2, y3));
    maxy = MAX(y1, MAX(y2, y3));

    /* oriznuti podle rozmeru okna */
    miny = MAX(miny, 0);
    maxy = MIN(maxy, pRenderer->frame_h - 1);
    minx = MAX(minx, 0);
    maxx = MIN(maxx, pRenderer->frame_w - 1);

    /* Pineduv alg. rasterizace troj.
       hranova fce je obecna rovnice primky Ax + By + C = 0
       primku prochazejici body (x1, y1) a (x2, y2) urcime jako
       (y1 - y2)x + (x2 - x1)y + x1y2 - x2y1 = 0 */

    /* normala primek - vektor kolmy k vektoru mezi dvema vrcholy, tedy (-dy, dx) */
    a1 = y1 - y2;
    a2 = y2 - y3;
    a3 = y3 - y1;
    b1 = x2 - x1;
    b2 = x3 - x2;
    b3 = x1 - x3;

    /* koeficient C */
    c1 = x1 * y2 - x2 * y1;
    c2 = x2 * y3 - x3 * y2;
    c3 = x3 * y1 - x1 * y3;

    /* vypocet hranove fce (vzdalenost od primky) pro protejsi body */
    s1 = a1 * x3 + b1 * y3 + c1;
    s2 = a2 * x1 + b2 * y1 + c2;
    s3 = a3 * x2 + b3 * y2 + c3;

    if ( !s1 || !s2 || !s3 )
    {
        return;
    }

    /* normalizace, aby vzdalenost od primky byla kladna uvnitr trojuhelniku */
    if( s1 < 0 )
    {
        a1 *= -1;
        b1 *= -1;
        c1 *= -1;
    }
    if( s2 < 0 )
    {
        a2 *= -1;
        b2 *= -1;
        c2 *= -1;
    }
    if( s3 < 0 )
    {
        a3 *= -1;
        b3 *= -1;
        c3 *= -1;
    }

    /* koeficienty pro barycentricke souradnice */
    alpha = 1.0 / ABS(s2);
    beta = 1.0 / ABS(s3);
    gamma = 1.0 / ABS(s1);

    /* vyplnovani... */
    for( y = miny; y <= maxy; ++y )
    {
        /* inicilizace hranove fce v bode (minx, y) */
        e1 = a1 * minx + b1 * y + c1;
        e2 = a2 * minx + b2 * y + c2;
        e3 = a3 * minx + b3 * y + c3;

        for( x = minx; x <= maxx; ++x )
        {
            if( e1 >= 0 && e2 >= 0 && e3 >= 0 )
            {
                /* interpolace pomoci barycentrickych souradnic
                   e1, e2, e3 je aktualni vzdalenost bodu (x, y) od primek */
                w1 = alpha * e2;
                w2 = beta * e3;
                w3 = gamma * e1;

                /* interpolace z-souradnice */
                tex_x = (w1 * (t1->x/h1) + w2 * (t2->x/h2) + w3 * (t3->x/h3))/(w1 * (1/h1) + w2 * (1/h2) + w3 * (1/h3));
                tex_y = (w1 * (t1->y/h1) + w2 * (t2->y/h2) + w3 * (t3->y/h3))/(w1 * (1/h1) + w2 * (1/h2) + w3 * (1/h3));

                z = w1 * v1->z + w2 * v2->z + w3 * v3->z;

               S_RGBA colr = studrenTextureValue(pRenderer, tex_x, tex_y);

                /* interpolace barvy */
                color.red = ROUND2BYTE((w1 * col1.red + w2 * col2.red + w3 * col3.red)) * (colr.red)/255;
                color.green = ROUND2BYTE((w1 * col1.green + w2 * col2.green + w3 * col3.green)) * (colr.green)/255;
                color.blue = ROUND2BYTE((w1 * col1.blue + w2 * col2.blue + w3 * col3.blue)) * (colr.blue)/255;
                color.alpha = 255;

                /* vykresleni bodu */
                if( z < DEPTH(pRenderer, x, y) )
                {
                    PIXEL(pRenderer, x, y) = color;
                    DEPTH(pRenderer, x, y) = z;
                }
            }

            /* hranova fce o pixel vedle */
            e1 += a1;
            e2 += a2;
            e3 += a3;
        }
    }
}

/******************************************************************************
 * Vykresli i-ty trojuhelnik n-teho klicoveho snimku modelu
 * pomoci nove fce studrenDrawTriangle()
 * Pred vykreslenim aplikuje na vrcholy a normaly trojuhelniku
 * aktualne nastavene transformacni matice!
 * i - index trojuhelniku
 * n - index klicoveho snimku (float pro pozdejsi interpolaci mezi snimky)
 */

void studrenProjectTriangle(S_Renderer *pRenderer, S_Model *pModel, int i, float n)
{
    S_Coords    aa, bb, cc;             /* souradnice vrcholu po transformaci */
    S_Coords    naa, nbb, ncc;          /* normaly ve vrcholech po transformaci */
    S_Coords    nn;                     /* normala trojuhelniku po transformaci */
    int         u1, v1, u2, v2, u3, v3; /* souradnice vrcholu po projekci do roviny obrazovky */
    S_Triangle  * triangle;
    int         vertexOffset, normalOffset; /* offset pro vrcholy a normalove vektory trojuhelniku */
    int         i0, i1, i2, in;             /* indexy vrcholu a normaly pro i-ty trojuhelnik n-teho snimku */

    double      h1, h2, h3;					/* homogenni souradnice */
    float       y = n - (int)n;				/* desitinna cast od n, pro interpolaci */
    int         i0_y, i1_y, i2_y, in_y;
    int         vertexOffset_y, normalOffset_y;

    S_Coords *old_coord[4],*new_coord[4],*old_coord_norm[4],*new_coord_norm[4], coord[4], coord_norm[4];

    IZG_ASSERT(pRenderer && pModel && i >= 0 && i < trivecSize(pModel->triangles) && n >= 0 );

    /* z modelu si vytahneme i-ty trojuhelnik */
    triangle = trivecGetPtr(pModel->triangles, i);
	/* ziskame offset pro vrcholy n-teho snimku */
    vertexOffset = (((int) n) % pModel->frames) * pModel->verticesPerFrame;
    vertexOffset_y = (((int) n+1) % pModel->frames) * pModel->verticesPerFrame;

	/* ziskame offset pro normaly trojuhelniku n-teho snimku */
    normalOffset = (((int) n) % pModel->frames) * pModel->triangles->size;
    normalOffset_y = (((int) n+1) % pModel->frames) * pModel->triangles->size;

    /* indexy vrcholu pro i-ty trojuhelnik n-teho snimku - pricteni offsetu */

    i0 = triangle->v[ 0 ] + vertexOffset;
    i1 = triangle->v[ 1 ] + vertexOffset;
    i2 = triangle->v[ 2 ] + vertexOffset;
 
    i0_y = triangle->v[ 0 ] + vertexOffset_y;
    i1_y = triangle->v[ 1 ] + vertexOffset_y;
    i2_y = triangle->v[ 2 ] + vertexOffset_y;

    /* index normaloveho vektoru pro i-ty trojuhelnik n-teho snimku - pricteni offsetu */
    in = triangle->n + normalOffset;
    in_y = triangle->n + normalOffset_y;

    int old_i_all[4] = { i0,i1,i2,in }, new_i_all[4] = {i0_y,i1_y,i2_y,in_y};

    for (int i=0 ; i < 4; i++) {
        if (i < 3) {
        old_coord[i] = cvecGetPtr(pModel->vertices, old_i_all[i]);
        new_coord[i] = cvecGetPtr(pModel->vertices, new_i_all[i]);
        old_coord_norm[i] = cvecGetPtr(pModel->normals, old_i_all[i]);
        new_coord_norm[i] = cvecGetPtr(pModel->normals, new_i_all[i]);

        coord[i].x = old_coord[i]->x + y *(new_coord[i]->x - old_coord[i]->x);
        coord[i].y = old_coord[i]->y + y *(new_coord[i]->y - old_coord[i]->y);
        coord[i].z = old_coord[i]->z + y *(new_coord[i]->z - old_coord[i]->z);

        coord_norm[i].x = old_coord_norm[i]->x + y *(new_coord_norm[i]->x - old_coord_norm[i]->x);
        coord_norm[i].y = old_coord_norm[i]->y + y *(new_coord_norm[i]->y - old_coord_norm[i]->y);
        coord_norm[i].z = old_coord_norm[i]->z + y *(new_coord_norm[i]->z - old_coord_norm[i]->z);
        }
        else {
        old_coord[i] = cvecGetPtr(pModel->trinormals, old_i_all[i]);
        new_coord[i] = cvecGetPtr(pModel->trinormals, new_i_all[i]);

        coord[i].x = old_coord[i]->x + y *(new_coord[i]->x - old_coord[i]->x);
        coord[i].y = old_coord[i]->y + y *(new_coord[i]->y - old_coord[i]->y);
        coord[i].z = old_coord[i]->z + y *(new_coord[i]->z - old_coord[i]->z);
        }
    }
    /* transformace vrcholu matici model */
    trTransformVertex(&aa, &coord[0]);
    trTransformVertex(&bb, &coord[1]);
    trTransformVertex(&cc, &coord[2]);

    /* promitneme vrcholy trojuhelniku na obrazovku */
    h1 = trProjectVertex(&u1, &v1, &aa);
    h2 = trProjectVertex(&u2, &v2, &bb);
    h3 = trProjectVertex(&u3, &v3, &cc);

    /* pro osvetlovaci model transformujeme take normaly ve vrcholech */
    trTransformVector(&naa, &coord_norm[0]);
    trTransformVector(&nbb, &coord_norm[1]);
    trTransformVector(&ncc, &coord_norm[2]);

    /* normalizace normal */
    coordsNormalize(&naa);
    coordsNormalize(&nbb);
    coordsNormalize(&ncc);

    /* transformace normaly trojuhelniku matici model */
    trTransformVector(&nn, &coord[3]);

    /* normalizace normaly */
    coordsNormalize(&nn);

    /* je troj. privraceny ke kamere, tudiz viditelny? */
    if( !renCalcVisibility(pRenderer, &aa, &nn) )
    {
        /* odvracene troj. vubec nekreslime */
        return;
    }

    S_Coords t1 = triangle->t[0];
    S_Coords t2 = triangle->t[1];
    S_Coords t3 = triangle->t[2];

    /* rasterizace trojuhelniku */
    studrenDrawTriangle(pRenderer,
                    &aa, &bb, &cc,
                    &naa, &nbb, &ncc,
                    u1, v1, u2, v2, u3, v3,
                    &t1, &t2, &t3,
                    h1, h2, h3
                    );
}

/******************************************************************************
* Vraci hodnotu v aktualne nastavene texture na zadanych
* texturovacich souradnicich u, v
* Pro urceni hodnoty pouziva bilinearni interpolaci
* Pro otestovani vraci ve vychozim stavu barevnou sachovnici dle uv souradnic
* u, v - texturovaci souradnice v intervalu 0..1, ktery odpovida sirce/vysce textury
*/

S_RGBA studrenTextureValue( S_StudentRenderer * pRenderer, double u, double v )
{
    if(isnan(u) || isnan(v)) return makeColor( 0, 0, 0 );

    int x1, x2, y1, y2;
    char cer_1, cer_2, cer, zel_1, zel_2, zel, mod_1, mod_2, mod;

    x1 = u * TEXTURE_SIZE;
    y1 = v * TEXTURE_SIZE;
    x2 = u * TEXTURE_SIZE + 1;
    y2 = v * TEXTURE_SIZE + 1;

    S_RGBA tecka_1 = pRenderer->texture[TEXTURE_SIZE * x1 + y1];
    S_RGBA tecka_2 = pRenderer->texture[TEXTURE_SIZE * x1 + y2];
    S_RGBA tecka_3 = pRenderer->texture[TEXTURE_SIZE * x2 + y1];
    S_RGBA tecka_4 = pRenderer->texture[TEXTURE_SIZE * x2 + y2];

    cer_1 = ((x2 - x1)/(x2 - x1))*tecka_1.red + ((x1 - x1)/(x2 - x1))*tecka_3.red;
    cer_2 = ((x2 - x1)/(x2 - x1))*tecka_2.red + ((x1 - x1)/(x2 - x1))*tecka_4.red;
    cer = ((y2 - y1)/(y2 - y1))*cer_1 + ((y1 - y1)/(y2 - y1))*cer_2;

    zel_1 = ((x2 - x1)/(x2 - x1))*tecka_1.green + ((x1 - x1)/(x2 - x1))*tecka_3.green;
    zel_2 = ((x2 - x1)/(x2 - x1))*tecka_2.green + ((x1 - x1)/(x2 - x1))*tecka_4.green;
    zel = ((y2 - y1)/(y2 - y1))*zel_1 + ((y1 - y1)/(y2 - y1))*zel_2;
   
    mod_1 = ((x2 - x1)/(x2 - x1))*tecka_1.blue + ((x1 - x1)/(x2 - x1))*tecka_3.blue;
    mod_2 = ((x2 - x1)/(x2 - x1))*tecka_2.blue + ((x1 - x1)/(x2 - x1))*tecka_4.blue;
    mod = ((y2 - y1)/(y2 - y1))*mod_1 + ((y1 - y1)/(y2 - y1))*mod_2;

    return makeColor(cer, zel, mod);
}

/******************************************************************************
 ******************************************************************************
 * Funkce pro vyrenderovani sceny, tj. vykresleni modelu
 * Upravte tak, aby se model vykreslil animovane
 * (volani renderModel s aktualizovanym parametrem n)
 */

void renderStudentScene(S_Renderer *pRenderer, S_Model *pModel)
{
    /* zaklad fce zkopirujte z main.c */
    const S_Material    MAT_WHITE_AMBIENT  = { 1, 1, 1, 1.0 };
    const S_Material    MAT_WHITE_DIFFUSE  = { 1, 1, 1, 1.0 };
    const S_Material    MAT_WHITE_SPECULAR = { 1, 1, 1, 1.0 };

    /* test existence frame bufferu a modelu */
    IZG_ASSERT(pModel && pRenderer);

    /* nastavit projekcni matici */
    trProjectionPerspective(pRenderer->camera_dist, pRenderer->frame_w, pRenderer->frame_h);

    /* vycistit model matici */
    trLoadIdentity();

    /* nejprve nastavime posuv cele sceny od/ke kamere */
    trTranslate(0.0, 0.0, pRenderer->scene_move_z);

    /* nejprve nastavime posuv cele sceny v rovine XY */
    trTranslate(pRenderer->scene_move_x, pRenderer->scene_move_y, 0.0);

    /* natoceni cele sceny - jen ve dvou smerech - mys je jen 2D... :( */
    trRotateX(pRenderer->scene_rot_x);
    trRotateY(pRenderer->scene_rot_y);

    /* nastavime material */
    renMatAmbient(pRenderer, &MAT_WHITE_AMBIENT);
    renMatDiffuse(pRenderer, &MAT_WHITE_DIFFUSE);
    renMatSpecular(pRenderer, &MAT_WHITE_SPECULAR);

    /* a vykreslime nas model (ve vychozim stavu kreslime pouze frame 0) */
    if (change == 1)
    {
        frame += 0.4;
        studrenClearBuffers(pRenderer);    
        change = 0;
    }
    renderModel(pRenderer, pModel, frame);
}

/* Callback funkce volana pri tiknuti casovace
 * ticks - pocet milisekund od inicializace */
void onTimer( int ticks )
{
    /* uprava parametru pouzivaneho pro vyber klicoveho snimku
     * a pro interpolaci mezi snimky */
    time_y = (ticks - time_z) - 33;
    
    if (time_y >= 0)
    {
        change = 1;
    }
    time_z = ticks;
}

/*****************************************************************************
 *****************************************************************************/
