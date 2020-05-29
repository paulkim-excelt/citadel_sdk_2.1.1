/*******************************************************************
 *
 *  Copyright 2009
 *  Broadcom Corporation
 *  5300 California Avenue
 *  Irvine CA 92617
 *
 *  Broadcom provides the current code only to licensees who
 *  may have the non-exclusive right to use or modify this
 *  code according to the license.
 *  This program may be not sold, resold or disseminated in
 *  any form without the explicit consent from Broadcom Corp
 *
 * cvfpstore.h: Broadcom CV DigitalPersona fingerprint recognition
 *              engine storage API
 *
 *******************************************************************/
#ifndef _CVFPSTORE_H_
#define _CVFPSTORE_H_
#ifdef __cplusplus
extern "C" {
#endif

/*
 * locally used type definition.
 */
typedef void * PVOID;

/*
 * Template record for each fingerprint object in CV object store.
 */
typedef struct _CV_FP_TEMPLATE_STORE {
    cv_obj_handle        pFPData;        /* CV FP Object handle  */
    DPFR_SUBJECT_ID      subjectId;      /* Subject ID           */
    DPFR_FINGER_POSITION fingerPosition; /* Finger position      */
} CV_FP_TEMPLATE_STORE, *PCV_FP_TEMPLATE_STORE;

/*
 * List of templates for fingerprint recognition engine.
 */
typedef struct _CV_FP_TEMPLATE_LIST {
    CV_FP_TEMPLATE_STORE  *fpStore;
    byte                  *cachedTemplate;
    uint32_t              cachedTemplateLength;
    byte                  *rawTemplate;
    uint32_t              rawTemplateLength;
    uint32_t              numTemplates;
    size_t                cursorLocation;
    cv_handle             cvHandle;
} CV_FP_TEMPLATE_LIST, *PCV_FP_TEMPLATE_LIST;

typedef struct _CV_FP_SUBJECT_LIST {
    DPFR_SUBJECT_ID subjectId;
} CV_FP_SUBJECT_LIST, *PCV_SP_SUBJECT_LIST;

/*
 * Prototype definitions.
 */
DPFR_RESULT cvFPSAInit(cv_handle cvHandle,
                       uint32_t templateCount,
                       PDPFR_STORAGE pDPStore,
                       cv_obj_handle *pTemplateHandles,
                       uint32_t templateLength,
                       byte *pTemplate);
DPFR_RESULT cvFPSAGetSizes(PVOID storageContext,
                           size_t *pNumFingers,
                           size_t *pNumSubjects);
DPFR_RESULT cvFPSAMoveToTemplate(PVOID storageContext,
                                 DPFR_FINGER_POSITION fingerPosition);
DPFR_RESULT cvFPSANextTemplate(PVOID storageContext);
DPFR_RESULT cvFPSAGetCurrentFinger(PVOID storageContext,
                                   DPFR_SUBJECT_ID subjectId,
                                   DPFR_FINGER_POSITION *pFingerPosition);
DPFR_RESULT cvFPSAGetCurrentTemplate(PVOID storageContext,
                                     DPFR_HANDLE hEngineContext,
                                     DPFR_HANDLE *phTemplate);
DPFR_RESULT cvFPSAHintCurrentCache(PVOID storageContext,
                                   int score);
DPFR_RESULT cvFPSAUpdateCurrentTemplate(PVOID storageContext,
                                        DPFR_HANDLE hTemplate);
DPFR_RESULT cvFPSALocate(PVOID storageContext,
                         DPFR_SUBJECT_ID subjectId,
                         DPFR_FINGER_POSITION fingerPosition);
DPFR_RESULT cvFPSAFreeTemplatesList(void);

/*
 * Typedefs for others.
 */
typedef DPFR_RESULT (*get_sizes)(PVOID storageContext,
                                 size_t *pNumFingers,
                                 size_t *pNumSubjects);
typedef DPFR_RESULT (*move_to)(PVOID storageContext,
                               DPFR_FINGER_POSITION fingerPosition);
typedef DPFR_RESULT (*next)(PVOID storageContext);
typedef DPFR_RESULT (*get_current_finger)(PVOID storageContext,
                                          DPFR_SUBJECT_ID subjectId,
                                          DPFR_FINGER_POSITION *pFingerPosition);
typedef DPFR_RESULT (*get_current_template)(PVOID storageContext,
                                            DPFR_HANDLE hEngineContext,
                                            DPFR_HANDLE *phTemplate);
typedef DPFR_RESULT (*hint_current_cache)(PVOID storageContext,
                                          int score);
typedef DPFR_RESULT (*update_current_template)(PVOID storageContext,
                                               DPFR_HANDLE hTemplate);
typedef DPFR_RESULT (*locate)(PVOID storageContext,
                              DPFR_SUBJECT_ID subjectId,
                              DPFR_FINGER_POSITION fingerPosition);
#ifdef __cplusplus
}
#endif /* __cplusplus */
#endif /* _CVFPSTORE_H_ */
