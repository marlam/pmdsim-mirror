/*
 * Copyright (C) 2013, 2014
 * Computer Graphics Group, University of Siegen, Germany.
 * http://www.cg.informatik.uni-siegen.de/
 * All rights reserved.
 * Written by Martin Lambers <martin.lambers@uni-siegen.de>.
 */

#include "trianglepatch.h"


TrianglePatch::TrianglePatch() : texture(0)
{
    for (int i = 0; i < 4; i++)
        for (int j = 0; j < 4; j++)
            transformation[i * 4 + j] = (i == j ? 1.0f : 0.0f);
}
