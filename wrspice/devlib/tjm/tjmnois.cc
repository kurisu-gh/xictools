
/*========================================================================*
 *                                                                        *
 *  Distributed by Whiteley Research Inc., Sunnyvale, California, USA     *
 *                       http://wrcad.com                                 *
 *  Copyright (C) 2019 Whiteley Research Inc., all rights reserved.       *
 *  Author: Stephen R. Whiteley, except as indicated.                     *
 *                                                                        *
 *  As fully as possible recognizing licensing terms and conditions       *
 *  imposed by earlier work from which this work was derived, if any,     *
 *  this work is released under the Apache License, Version 2.0 (the      *
 *  "License").  You may not use this file except in compliance with      *
 *  the License, and compliance with inherited licenses which are         *
 *  specified in a sub-header below this one if applicable.  A copy       *
 *  of the License is provided with this distribution, or you may         *
 *  obtain a copy of the License at                                       *
 *                                                                        *
 *        http://www.apache.org/licenses/LICENSE-2.0                      *
 *                                                                        *
 *  See the License for the specific language governing permissions       *
 *  and limitations under the License.                                    *
 *                                                                        *
 *   THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,      *
 *   EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES      *
 *   OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-        *
 *   INFRINGEMENT.  IN NO EVENT SHALL WHITELEY RESEARCH INCORPORATED      *
 *   OR STEPHEN R. WHITELEY BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER     *
 *   LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,      *
 *   ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE       *
 *   USE OR OTHER DEALINGS IN THE SOFTWARE.                               *
 *                                                                        *
 *========================================================================*
 *               XicTools Integrated Circuit Design System                *
 *                                                                        *
 * WRspice Circuit Simulation and Analysis Tool:  Device Library          *
 *                                                                        *
 *========================================================================*
 $Id:$
 *========================================================================*/

#include <stdio.h>
#include "tjmdefs.h"
#include "noisdefs.h"


// Add flicker noise?  Length and width parameters don't make sense
// for JJ.
//#define WITH_KF

#ifdef WITH_KF
/*-----------------------------------------------------------------------------
Flicker noise model

Noise(sid) = (KF * I^AF) / (Leff^LF * Weff^WF * f^EF)

Noise(sid)  Noise spectrum density  A^2Hz
I           Current                 A
Leff        Eff length (Ldrawn-dL)  m
Weff        Eff width  (Wdrawn-dw)  m
f           Frequency               Hz

KF          Flicker noise coefficient   0  >= 0
AF          exponent of current         2  > 0
LF          exp of eff length           1  > 0
WF          exp of eff width            1  > 0
EF          exp of frequency            1  > 0

Requires effective l, w params be given.
------------------------------------------------------------------------------*/


namespace
{
    inline bool doing_flicker(sTJMmodel *model, sTJMinstance *inst)
    {
        return (model->TJMkfGiven && model->TJMkf != 0.0 &&
            inst->TJMlength > 0.0 && inst->TJMwidth > 0.0);
    }
}
#endif


//    This function names and evaluates all of the noise sources
//    associated with resistors.  It starts with the model *firstModel
//    and traverses all of its instances.  It then proceeds to any other
//    models on the linked list.  The total output noise density
//    generated by all the resistors is summed in the variable "OnDens".
//
int
TJMdev::noise (int mode, int operation, sGENmodel *genmod, sCKT *ckt,
    sNdata *data, double *OnDens)
{
    sTJMmodel *model = static_cast<sTJMmodel*>(genmod);
    char resname[N_MXVLNTH];
    if (operation == N_OPEN) {
        // see if we have to to produce a summary report
        // if so, name the noise generator
        if (static_cast<sNOISEAN*>(ckt->CKTcurJob)->NStpsSm == 0)
            return (OK);
        if (mode == N_DENS) {
            for ( ; model; model = model->next()) {
                sTJMinstance *inst;
                for (inst = model->inst(); inst; inst = inst->next()) {

                    sprintf(resname, "onoise_%s", (char*)inst->GENname);
                    Realloc(&data->namelist, data->numPlots+1,
                        data->numPlots);
                    ckt->newUid(&data->namelist[data->numPlots++],
                        0, resname, UID_OTHER);
                        // we've added one more plot

#ifdef WITH_KF
                    if (doing_flicker(model, inst)) {
                        sprintf(resname, "onoise_%s_kf", (char*)inst->GENname);
                        Realloc(&data->namelist, data->numPlots+1,
                            data->numPlots);
                        ckt->newUid(&data->namelist[data->numPlots++],
                            0, resname, UID_OTHER);
                            // we've added one more plot
                    }
#endif
                }
            }
            return (OK);
        }

        if (mode == INT_NOIZ) {
            for ( ; model; model = model->next()) {
                sTJMinstance *inst;
                for (inst = model->inst(); inst; inst = inst->next()) {

                    sprintf(resname, "onoise_total_%s",
                        (char*)inst->GENname);
                    Realloc(&data->namelist, data->numPlots+2,
                        data->numPlots);
                    ckt->newUid(&data->namelist[data->numPlots++],
                        0, resname, UID_OTHER);
                    sprintf(resname, "inoise_total_%s",
                        (char*)inst->GENname);
                    ckt->newUid(&data->namelist[data->numPlots++],
                        0, resname, UID_OTHER);
                    // we've added two more plots

#ifdef WITH_KF
                    if (doing_flicker(model, inst)) {
                        sprintf(resname, "onoise_total_%s_kf",
                            (char*)inst->GENname);
                        Realloc(&data->namelist, data->numPlots+2,
                            data->numPlots);
                        ckt->newUid(&data->namelist[data->numPlots++],
                            0, resname, UID_OTHER);
                        sprintf(resname, "inoise_total_%s_kf",
                            (char*)inst->GENname);
                        ckt->newUid(&data->namelist[data->numPlots++],
                            0, resname, UID_OTHER);
                        // we've added two more plots
                    }
#endif
                }
            }
        }
        return (OK);
    }

    if (operation == N_CALC) {
        if (mode == N_DENS) {
            for ( ; model; model = model->next()) {
                sTJMinstance *inst;
                for (inst = model->inst(); inst; inst = inst->next()) {

                    double noizDens[2];
                    double lnNdens[2];
                    NevalSrc(&noizDens[0], &lnNdens[0], ckt, THERMNOISE,
                        inst->TJMposNode, inst->TJMnegNode,
                        inst->TJMnoise * inst->TJMgqp);

                    *OnDens += noizDens[0];
#ifdef WITH_KF
                    if (doing_flicker(model, inst)) {
                        double cur =
                            inst->TJMv * inst->TJMnoise * inst->TJMgqp;
                        if (!model->TJMafGiven)
                            cur *= cur;
                        else if (model->TJMaf != 1.0)
                            cur = pow(cur, model->TJMaf);
                        double el = inst->TJMlength - model->TJMshorten;
                        if (model->TJMlfGiven && model->TJMlf != 1.0)
                            el = pow(el, model->TJMlf);
                        double ew = inst->TJMwidth - model->TJMnarrow;
                        if (model->TJMwfGiven && model->TJMwfGiven != 1.0)
                            ew = pow(ew, model->TJMwf);
                        double freq = data->freq;
                        if (model->TJMefGiven && model->TJMef != 1.0)
                            freq = pow(freq, model->TJMefGiven);

                        noizDens[1] = (model->TJMkf*cur)/(el*ew*freq);
                        lnNdens[1] = log(SPMAX(noizDens[1], N_MINLOG));
                        *OnDens += noizDens[1];
                    }
#endif

                    if (data->delFreq == 0.0) { 

                        // if we haven't done any previous integration,
                        // we need to initialize our "history" variables

                        inst->TJMnVar[LNLSTDENS][0] = lnNdens[0];
#ifdef WITH_KF
                        if (doing_flicker(model, inst))
                            inst->TJMnVar[LNLSTDENS][1] = lnNdens[1];
#endif

                        // clear out our integration variable if it's the
                        // first pass

                        if (data->freq ==
                                ((sNOISEAN*)ckt->CKTcurJob)->JOBac.fstart()) {
                            inst->TJMnVar[OUTNOIZ][0] = 0.0;
                            inst->TJMnVar[OUTNOIZ][1] = 0.0;
                        }
                    }
                    else {
                        // data->delFreq != 0.0 (we have to integrate)
                        double tempOutNoise;
                        double tempInNoise;
                        tempOutNoise =
                            data->integrate(noizDens[0], lnNdens[0],
                            inst->TJMnVar[LNLSTDENS][0]);
                        tempInNoise =
                            data->integrate(noizDens[0]*data->GainSqInv,
                            lnNdens[0] + data->lnGainInv,
                            inst->TJMnVar[LNLSTDENS][0] + data->lnGainInv);

                        data->outNoiz += tempOutNoise;
                        data->inNoise += tempInNoise;
                        inst->TJMnVar[LNLSTDENS][0] = lnNdens[0];
                        inst->TJMnVar[OUTNOIZ][0] += tempOutNoise;
                        inst->TJMnVar[INNOIZ][0] += tempInNoise;

#ifdef WITH_KF
                        if (doing_flicker(model, inst)) {
                            tempOutNoise =
                                data->integrate(noizDens[1], lnNdens[1],
                                inst->TJMnVar[LNLSTDENS][1]);
                            tempInNoise =
                                data->integrate(noizDens[1]*data->GainSqInv,
                                lnNdens[1] + data->lnGainInv,
                                inst->TJMnVar[LNLSTDENS][1] + data->lnGainInv);

                            data->outNoiz += tempOutNoise;
                            data->inNoise += tempInNoise;
                            inst->TJMnVar[LNLSTDENS][1] = lnNdens[1];
                            inst->TJMnVar[OUTNOIZ][1] += tempOutNoise;
                            inst->TJMnVar[INNOIZ][1] += tempInNoise;
                        }
#endif
                    }
                    if (data->prtSummary) {
                        data->outpVector[data->outNumber++] = noizDens[0];
#ifdef WITH_KF
                        if (doing_flicker(model, inst))
                            data->outpVector[data->outNumber++] = noizDens[1];
#endif
                    }
                }
            }
            return (OK);
        }

        if (mode == INT_NOIZ) {
            if (static_cast<sNOISEAN*>(ckt->CKTcurJob)->NStpsSm == 0)
                return (OK);
            for ( ; model; model = model->next()) {
                sTJMinstance *inst;
                for (inst = model->inst(); inst; inst = inst->next()) {

                    // already calculated, just output
                    data->outpVector[data->outNumber++] =
                        inst->TJMnVar[OUTNOIZ][0];
                    data->outpVector[data->outNumber++] =
                        inst->TJMnVar[INNOIZ][0];
#ifdef WITH_KF
                    if (doing_flicker(model, inst)) {
                        data->outpVector[data->outNumber++] =
                            inst->TJMnVar[OUTNOIZ][1];
                        data->outpVector[data->outNumber++] =
                            inst->TJMnVar[INNOIZ][1];
                    }
#endif
                }
            }
        }
    }
    return (OK);
}

