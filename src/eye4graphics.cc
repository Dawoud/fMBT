/*
 * fMBT, free Model Based Testing tool
 * Copyright (c) 2012, Intel Corporation.
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms and conditions of the GNU Lesser General Public License,
 * version 2.1, as published by the Free Software Foundation.
 *
 * This program is distributed in the hope it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public License for
 * more details.
 *
 * You should have received a copy of the GNU Lesser General Public License along with
 * this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin St - Fifth Floor, Boston, MA 02110-1301 USA.
 *
 */

#include "eye4graphics.h"
#include <math.h>
#include <algorithm>
#include <vector>
#include <climits>

#include <Magick++.h>
using namespace Magick;

#define COLORS 64

#define INCOMPARABLE -1

inline bool same_color(const PixelPacket *p1, const PixelPacket *p2,
                       const int colorDiff, const unsigned char skipTransparency)
{
    /* do not compare transparent pixels */
    if (skipTransparency) {
        if (p1->opacity >= skipTransparency || p2->opacity >= skipTransparency) {
            return true;
        }
    }

    if ((unsigned char)(p1->red) + colorDiff   >= (unsigned char)(p2->red) &&
        (unsigned char)(p2->red) + colorDiff   >= (unsigned char)(p1->red) &&
        (unsigned char)(p1->green) + colorDiff >= (unsigned char)(p2->green) &&
        (unsigned char)(p2->green) + colorDiff >= (unsigned char)(p1->green) &&
        (unsigned char)(p1->blue) + colorDiff  >= (unsigned char)(p2->blue) &&
        (unsigned char)(p2->blue) + colorDiff  >= (unsigned char)(p1->blue)) {
        return true;
    } else {
        return false;
    }
}

bool pixelperfect_match(const int hayxsize,
                        const int neex, const int neey,
                        const int x, const int y,
                        const PixelPacket *hay_pixel,
                        const PixelPacket *nee_pixel,
                        const int colorDiff,
                        const unsigned char skipTransparency)
{
    /* try to fail fast, do very fast diagonal sweep */
    int samples = 8;
    int stepx = neex >= samples ? neex/samples : 1;
    int stepy = neey >= samples ? neey/samples : 1;
    if (stepx > 16) stepx = 16;
    if (stepy > 16) stepy = 16;
    for (int _x=0, _y=0;
         _x<neex && _y<neey;
         _x+=stepx, _y+=stepy) {
        if (!same_color((hay_pixel + hayxsize*(y+_y) + x+_x),
                        (nee_pixel + neex*_y     + _x),
                        colorDiff,
                        skipTransparency)) {
            return false;
        }
    }

    /* take more samples from the middle row */
    samples = 32;
    stepx = neex >= samples ? neex/samples : 1;
    if (stepx > 8) stepx = 8;
    for (int _x=0, _y=neey/2;
         _x<neex;
         _x+=stepx) {
        if (!same_color((hay_pixel + hayxsize*(y+_y) + x+_x),
                        (nee_pixel + neex*_y     + _x),
                        colorDiff,
                        skipTransparency)) {
            return false;
        }
    }

    /* full check, pixel-by-pixel */
    for(int _y=0; _y<neey; _y++) {
        for(int _x=0; _x<neex; _x++) {
            if (!same_color((hay_pixel + hayxsize*(y+_y) + x+_x),
                            (nee_pixel + neex*_y     + _x),
                            colorDiff,
                            skipTransparency)) {
                return false;
            }
        }
    }
    return true;
}

long normdiag_error(int hayxsize,int neex,int neey,int x,int y,
                    const PixelPacket *hay_pixel,
                    const PixelPacket *nee_pixel,
                    const int xstart, const int ystart,
                    const int xstep, const int ystep, int debug=0)
{
    /*
      +--+      +--+
      |n1|      |h1|           minimise
      +--+--+   +--+--+--+--+  ((n1-n2) - (h1-h2))^2 + penalty
         |  |      | ?| ?| ?|
         +--+--+   +--+--+--+
            |n2|   | ?|h2| ?|
            +--+   +--+--+--+
                   | ?| ?| ?|
                   +--+--+--+
    */

    // Normalise and quantify colors on searched area

    const int colors = COLORS; // Quantify to this number of colors
    int nee_greatest = -1;
    int nee_smallest = -1;
    int hay_greatest = -1;
    int hay_smallest = -1;
    for(int norx=xstart, nory=ystart;
        0 <= norx && 0 <= nory && norx < neex && nory < neey;
        norx+=xstep, nory+=ystep) {
        int nvalue = (*(nee_pixel+neex*nory+norx)).green;
        if (nee_smallest == -1 || nvalue < nee_smallest) nee_smallest = nvalue;
        if (nee_greatest == -1 || nvalue > nee_greatest) nee_greatest = nvalue;
        int hvalue = (*(hay_pixel+hayxsize*(nory+y)+norx+x)).green;
        if (hay_smallest == -1 || hvalue < hay_smallest) hay_smallest = hvalue;
        if (hay_greatest == -1 || hvalue > hay_greatest) hay_greatest = hvalue;
    }
    // If color ranges are too different, bail out.
    if (hay_greatest - hay_smallest > (nee_greatest - nee_smallest) * 2 ||
        nee_greatest - nee_smallest > (hay_greatest - hay_smallest) * 2) {
        return INCOMPARABLE;
    }
    const int nee_colorscale = nee_greatest == nee_smallest ?  1 : nee_greatest - nee_smallest;
    const int hay_colorscale = hay_greatest == hay_smallest ?  1 : hay_greatest - hay_smallest;

    long delta = 0;
    int checked_steps = 0;
    for(int checkx=xstart+1, checky=ystart+1;
        (0 <= checkx &&
         0 <= checky &&
         0 <= checkx+xstep*2-1 &&
         0 <= checky+ystep*2-1 &&
         checkx+xstep*2+1 < neex &&
         checky+ystep*2+1 < neey);
        checkx+=xstep, checky+=ystep) {

        int n1 = (((*(nee_pixel+neex*checky+checkx)).green - nee_smallest)*colors)
            / nee_colorscale;
        int n2 = (((*(nee_pixel+neex*(checky+ystep*2)+(checkx+xstep*2))).green - nee_smallest)*colors)
            / nee_colorscale;
        int bestdiff = -1;

        int h1, h2;
        int diff;

#define GREEN(h1xoffset, h1yoffset, penalty)                            \
        do {                                                            \
            h2=((((*(hay_pixel+hayxsize*(checky+y+(h1yoffset))+checkx+x+(h1xoffset))).green - hay_smallest)*colors) / hay_colorscale); \
            diff = (n1-n2)-(h1-h2); diff = diff*diff + penalty;         \
            if (bestdiff == -1 || diff < bestdiff) bestdiff = diff;     \
        } while (0);

        h1 = ((((*(hay_pixel+hayxsize*(checky+y)+checkx+x)).green - hay_smallest)*colors) / hay_colorscale);

        GREEN(xstep*2, ystep*2, 0);
        for (int xi=xstep*2-1; xi<=xstep*2+1 && bestdiff > colors/4; xi++) {
            for (int yi=ystep*2-1; yi<=ystep*2+1 && bestdiff > colors/4; yi++) {
                GREEN(xi, yi, colors/4);
            }
        }
        delta += bestdiff;
        checked_steps += 1;
    }

    return delta / checked_steps;
}

/*
 * iconsearch
 *
 * Parameters:
 *     retval    - vector of bounding boxes of found icons (out-parameter)
 *     haystack  - image from which icon is searched from
 *     needle    - icon to be searched for
 *     threshold - maximal level of difference between
 *                 found icon and original icon.
 *                     0  - pixel perfect
 *                     10 - big differences allowed
 *                 The smaller the threshold the faster the search.
 *
 * Return value:
 *     1         - icon candidate found
 *     0         - icon not found
 */
int iconsearch(std::vector<BoundingBox>& retval,
               Image& haystack,
               Image& needle,
               const int threshold,
               const double colorMatch,
               const double opacityLimit)
{
    const int color_threshold = COLORS * threshold;

    const int colorDiff = 256 - (256 * colorMatch);

    const unsigned char skipTransparency = 255 * opacityLimit;

    typedef std::pair<long, std::pair<int,int> > Candidate;
    std::vector< Candidate > candidates;

    haystack.modifyImage();
    needle.modifyImage();

    if (threshold == 0) {
        if (skipTransparency > 0) {
            haystack.type(TrueColorMatteType);
            needle.type(TrueColorMatteType);
        } else {
            haystack.type(TrueColorType);
            needle.type(TrueColorType);
        }
    } else {
        haystack.type(GrayscaleType);
        needle.type(GrayscaleType);
    }
    int hayx = haystack.columns(), hayy = haystack.rows();
    int neex = needle.columns(), neey = needle.rows();
    const PixelPacket *hay_pixel=haystack.getConstPixels(0,0,hayx,hayy);
    const PixelPacket *nee_pixel=needle.getConstPixels(0,0,neex,neey);

    if (threshold == 0) {
        /* Pixel-perfect match */
        int match_count = 0;
        for (int y=0; y < hayy-neey; y++) {
            for (int x=0; x < hayx-neex; x++) {
                if (pixelperfect_match(hayx, neex, neey, x, y,
                                       hay_pixel, nee_pixel,
                                       colorDiff,
                                       skipTransparency)) {
                    BoundingBox bbox;
                    bbox.left = x;
                    bbox.top = y;
                    bbox.right = x + neex;
                    bbox.bottom = y + neey;
                    bbox.error = 0;
                    retval.push_back(bbox);
                    match_count++;
                }
            }
        }
        return match_count > 0 ? 1 : 0;
    }
    
    /* Fuzzy match */
    /* sweep diagonal */
    int samples = 16;
    int stepx = neex >= samples ? neex/samples : 1;
    int stepy = neey >= samples ? neey/samples : 1;
    for (int y=0;y<hayy-neey;y++) {
        for (int x=0;x<hayx-neex;x++) {
            long thisdelta = normdiag_error(hayx, neex, neey, x, y,
                                            hay_pixel, nee_pixel,
                                            0, 0, stepx, stepy);
            if (thisdelta != INCOMPARABLE && thisdelta <= color_threshold) {
                candidates.push_back( Candidate(thisdelta, std::pair<int,int>(x, y)) );
            }
        }
    }

    /* sweep more lines, take samples on every pixel */
    for (unsigned int ci=0; ci < candidates.size(); ci++) {
        if (candidates[ci].first == LONG_MAX) continue;

        int thisdelta;
        int x = candidates[ci].second.first;
        int y = candidates[ci].second.second;

        thisdelta = normdiag_error(hayx, neex, neey, x, y, hay_pixel, nee_pixel,
                                   0, neey/2, 1, 0);
        if (thisdelta == INCOMPARABLE) {
            candidates[ci].first = LONG_MAX; continue;
        }
        candidates[ci].first += thisdelta;

        thisdelta = normdiag_error(hayx, neex, neey, x, y, hay_pixel, nee_pixel,
                                  neex/2, 0, 0, 1);
        if (thisdelta == INCOMPARABLE) {
            candidates[ci].first = LONG_MAX; continue;
        }
        candidates[ci].first += thisdelta;
        thisdelta = normdiag_error(hayx, neex, neey, x, y, hay_pixel, nee_pixel,
                                    neex-1, 0, -1, 1);
        if (thisdelta == INCOMPARABLE) {
            candidates[ci].first = LONG_MAX; continue;
        }
        candidates[ci].first += thisdelta;
    }

    if (candidates.size() > 0) {
        std::sort(candidates.begin(), candidates.end());
        BoundingBox bbox;
        bbox.left = candidates[0].second.first;
        bbox.top = candidates[0].second.second;
        bbox.right = bbox.left + neex;
        bbox.bottom = bbox.top + neey;
        bbox.error = candidates[0].first / COLORS / 4;
        retval.push_back(bbox);
        return 1;
    }
    else return 0;
}

int findSingleIcon(BoundingBox* bbox,
                   const char* imagefile,
                   const char* iconfile,
                   const int threshold,
                   const double colorMatch,
                   const double opacityLimit)
{
    /* TODO: another version with multiple versions of the same
     * icon. Clear, blurred, etc.
     */
    Image *haystack;
    Image *needle;

    int retval = 0;

    bbox->left   = -1;
    bbox->top    = -1;
    bbox->right  = -1;
    bbox->bottom = -1;
    bbox->error  = -1;

    try { haystack = new Image(imagefile); }
    catch(ErrorFileOpen e) {
        return -3;
    }

    try { needle = new Image(iconfile); }
    catch(ErrorFileOpen e) {
        delete haystack;
        return -4;
    }

    std::vector<BoundingBox> found;
    if (iconsearch(found, *haystack, *needle, threshold, colorMatch, opacityLimit) > 0
        && found.size() > 0) {
        *bbox = found[0];
        if (bbox->error > threshold)
            retval = -2;
        else
            retval = 0;
    } else {
        retval = -1;
    }
    delete needle;
    delete haystack;
    return retval;
}

int imageDimensions(BoundingBox* bbox,
                    const char* imagefile)
{
    Image *image;

    bbox->left   = 0;
    bbox->top    = 0;
    bbox->right  = -1;
    bbox->bottom = -1;
    bbox->error  = 0;
    
    try { image = new Image(imagefile); }
    catch(ErrorFileOpen e) {
        return -3;
    }

    bbox->right = image->columns();
    bbox->bottom = image->rows();

    delete image;

    return 0;
}


// make library:
// g++ -fPIC -shared -o eye4graphics.so -O3 -Wall `pkg-config --cflags --libs Magick++` eye4graphics.cc


/*
 * g++ -o eye4graphics -O3 -Wall `pkg-config --cflags --libs Magick++` eye4graphics.cc
 */

/*
int main(int argc,char** argv)
{
    BoundingBox box;

    int retval = findSingleIcon(&box, argv[1], argv[2], 4);

    printf("%i %i %i %i %i retval=%i\n", box.left, box.top, box.right, box.bottom, box.error, retval);

    return 0;
}
*/
