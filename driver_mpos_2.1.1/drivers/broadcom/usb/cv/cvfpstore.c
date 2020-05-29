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
 * cvfpstore.c: Broadcom CV DigitalPersona fingerprint recognition
 *              engine storage API
 *
 *******************************************************************/
#include "volatile_mem.h"
#include "cvmain.h"
#include "console.h"
#include "cvfpstore.h"

/*
 * Global variables for storage interface
 */
#define pFPTemplateList ((PCV_FP_TEMPLATE_LIST)VOLATILE_MEM_PTR->pFPTemplateList)

/*
 * cvFPSAGetSubjectId
 *
 * IN     index
 * IN/OUT pSubjectId
 *
 * Description:
 * This routine will return the subject ID for a given index
 * Subject IDs are fixed in a table. If the index is more than
 * list of subjects, then the subject ID is set to all Fs.
 *
 * Returns: None
 */
void cvFPSAGetSubjectId(uint32_t index, DPFR_SUBJECT_ID *pSubjectId)
{
    CV_FP_SUBJECT_LIST fpSubjects[MAX_FP_TEMPLATES] =
    {{0}, {1},  {2},  {3},  {4},  {5},  {6},  {7},  {8},  {9},
    {10}, {11}, {12}, {13}, {14}, {15}, {16}, {17}, {18}, {19},
    {20}, {21}, {22}, {23}, {24}, {25}, {26}, {27}, {28}, {29},
    {30}, {31}, {32}, {33}, {34}, {35}, {36}, {37}, {38}, {39},
    {40}, {41}, {42}, {43}, {44}, {45}, {46}, {47}, {48}, {49}};

    if (index < MAX_FP_TEMPLATES) {
        if (pSubjectId)
        {
            const void *pSubId = (void *)&fpSubjects[index];
            memcpy(pSubjectId, pSubId,
                   sizeof(DPFR_SUBJECT_ID));
            return;
        }
    } else {
        if (pSubjectId)
        {
            memset(pSubjectId, 'f', sizeof(DPFR_SUBJECT_ID));
        }
    }
}

/*
 * cvFPSAInit
 *
 * IN/OUT pListStore
 *
 * Description:
 * This routine will initialize the CV_FP_TEMPLATE_LIST data
 * structure and associated data. It will retrieve the
 * fingerprint objects from the CV object store and create
 * a table for easy access to the recognition engine.
 *
 * Returns: FR_OK FR_ERR_CANNOT_CREATE_DB
 */
DPFR_RESULT cvFPSAInit(cv_handle cvHandle,
                       uint32_t templateCount,
                       PDPFR_STORAGE pDPStore,
                       cv_obj_handle *pTemplateHandles,
                       uint32_t templateLength,
                       byte *pRawTemplate)
{
    DPFR_RESULT status = FR_OK;
    cv_status   cvStatus = CV_SUCCESS;
    cv_obj_properties        objProperties;
    uint32_t                 count = 0, count_valid=0;
    cv_obj_value_fingerprint *pTemplate;
    cv_obj_hdr               *pObj;

    /* Check input parameters validity */
    if (((templateCount == 0) ||
        (templateCount > MAX_FP_TEMPLATES)) ||
        !pDPStore)
    {
        return CV_INVALID_INPUT_PARAMETER;
    }
	
	printf("cvFPSAInit\n");

    pFPTemplateList =
         (PCV_FP_TEMPLATE_LIST) cv_malloc(sizeof(CV_FP_TEMPLATE_LIST));
    if (!pFPTemplateList)
    {
        return CV_VOLATILE_MEMORY_ALLOCATION_FAIL;
    }

    /* Zero the table memory for use. */
    memset(pFPTemplateList, 0, sizeof(CV_FP_TEMPLATE_LIST));

    /* Now allocate the storage for the list. */
    pFPTemplateList->numTemplates = templateCount;

    /* First check if it is a single template with raw data */
    if (templateLength)
    {
        pFPTemplateList->rawTemplate = pRawTemplate;
        pFPTemplateList->rawTemplateLength = templateLength;
        pFPTemplateList->numTemplates = templateCount;
        pFPTemplateList->cvHandle = cvHandle;
        pDPStore->storageContext = (void *)pFPTemplateList;
        return status;
    }

    pFPTemplateList->fpStore =
     (PCV_FP_TEMPLATE_STORE) cv_malloc(sizeof(CV_FP_TEMPLATE_STORE) * templateCount);
    if (!pFPTemplateList->fpStore)
    {
        cv_free(pFPTemplateList, sizeof(CV_FP_TEMPLATE_LIST));
        return CV_VOLATILE_MEMORY_ALLOCATION_FAIL;
    }
 
	printf("templatecount:%d\n",templateCount);

    /* Loop through the fingerprint objects and make a list */
    for (count = 0; count < templateCount; count++)
    {
    //    printf("Checking template %d handle 0x%x\n", count, pTemplateHandles[count]);
        printf("Checking template %d\n", count);
        memset(&objProperties, 0, sizeof(cv_obj_properties));
        objProperties.session = (cv_session *)cvHandle;
        objProperties.objHandle = pTemplateHandles[count];

        /* Get the object from CV storage */
        cvStatus = cvGetObj(&objProperties, &pObj);
        if (cvStatus != CV_SUCCESS)
        {
			printf("ERROR cvFPSAInit cvGetObj returned %d", cvStatus);
			if (pFPTemplateList->fpStore)
            {
                cv_free(pFPTemplateList->fpStore,
                        sizeof(CV_FP_TEMPLATE_STORE) * templateCount);
            }
            cv_free(pFPTemplateList, sizeof(CV_FP_TEMPLATE_LIST));
            return cvStatus;
        }

        /* Get the object pointers */
        cvFindObjPtrs(&objProperties, pObj);
        pTemplate = (cv_obj_value_fingerprint *)objProperties.pObjValue;
        if (pTemplate->type != CV_FINGERPRINT_TEMPLATE)
        {
            continue;
            /* Not a fingerprint object */
#if 0
            if (pFPTemplateList->fpStore)
            {
                cv_free(pFPTemplateList->fpStore,
                        sizeof(CV_FP_TEMPLATE_STORE) * templateCount);
            }
            cv_free(pFPTemplateList, sizeof(CV_FP_TEMPLATE_LIST));
            return CV_INVALID_OBJECT_TYPE;
#endif
        }
        cvUpdateObjCacheLRUFromHandle(objProperties.objHandle);
        /* Make a copy of the template */
        pFPTemplateList->fpStore[count_valid].pFPData =
                                   (cv_obj_handle)pTemplateHandles[count];
        pFPTemplateList->fpStore[count_valid].fingerPosition = 0;
        cvFPSAGetSubjectId(count_valid, 
                  (DPFR_SUBJECT_ID *)&(pFPTemplateList->fpStore[count_valid].subjectId));
/*
        memcpy(pFPTemplateList->fpStore[count].subjectId,
               fpSubjects[count].subjectId, sizeof(DPFR_SUBJECT_ID));
*/
        count_valid++;
    }

    /* valid templete. */
    pFPTemplateList->numTemplates = count_valid;

    /* Initial cursor location */
    pFPTemplateList->cursorLocation = 0;
    /* cached template data */
    pFPTemplateList->cachedTemplate = NULL;
    pFPTemplateList->cachedTemplateLength = 0;
    pFPTemplateList->cvHandle = cvHandle;

    /* return the storage context to the caller */
    pDPStore->storageContext = (void *)pFPTemplateList;

    return status;
}

/*
 * cvFPSAGetSizes
 *
 * IN   storageContext
 * OUT  pNumFingers
 * OUT  pNumSubjects
 *
 * Description:
 * This routine returns the number of templates in the list
 * using the output variable pNumSubjects passed in by the
 * caller and the pNumFingers is always set to 1.
 *
 * Returns: FR_OK FR_ERR_INVALID_PARAM
 */
DPFR_RESULT cvFPSAGetSizes(PVOID storageContext,
                           size_t *pNumFingers,
                           size_t *pNumSubjects)
{
    DPFR_RESULT status = FR_OK;
    PCV_FP_TEMPLATE_LIST pStorageContext =
                            (PCV_FP_TEMPLATE_LIST)storageContext;
    size_t i;
    size_t j;

    /* Validate the pointers passed */
    if (!pStorageContext ||
         pStorageContext != pFPTemplateList)
    {
        return FR_ERR_INVALID_PARAM;
    }

    if ( pNumFingers ) {
        *pNumFingers = pStorageContext->numTemplates;
    }

    /* Handle the case with a single raw template. */
    if (pStorageContext->rawTemplateLength != 0)
    {
        *pNumSubjects = pStorageContext->numTemplates;
        return status;
    }

    /* Check if we have any duplicates */
    if ( pNumSubjects )
    {
        *pNumSubjects = pStorageContext->numTemplates;
        for ( i = 1; i < pStorageContext->numTemplates; i++ )
        {
            for ( j = 0; j < i; j++ )
            {
                if (0 == memcmp(pStorageContext->fpStore[i].subjectId,
                                pStorageContext->fpStore[j].subjectId,
                                sizeof(DPFR_SUBJECT_ID)))
                {
                    /* Duplicate subject found... */
                    *pNumSubjects -= 1;
                    break;
                }
            }
        }
    }

    return status;
}

/*
 * cvFPSALocate
 *
 * IN   storageContext
 * IN   fingerPosition
 * IN   subjectId
 *
 * Description:
 * Routine to locate the template based on subject ID and
 * fingerposition.
 *
 * Returns: FR_OK FR_WRN_EOF FR_ERR_INVALID_PARAM
 */
DPFR_RESULT cvFPSALocate(PVOID storageContext,
                         DPFR_SUBJECT_ID subjectId,
                         DPFR_FINGER_POSITION fingerPosition)
{
    DPFR_RESULT status = FR_OK;
    PCV_FP_TEMPLATE_LIST pStorageContext =
                           (PCV_FP_TEMPLATE_LIST)storageContext;
    PCV_FP_TEMPLATE_STORE pTemplateData = NULL;
    size_t           i = 0;

    /* Validate the pointers passed */
    if (!pStorageContext ||
         pStorageContext != pFPTemplateList)
    {
        return FR_ERR_INVALID_PARAM;
    }

    pStorageContext->cursorLocation = 0;

    /* Handle the single raw template case. */
    if (pStorageContext->rawTemplateLength != 0)
    {
        return status;
    }

    /* Move to the first instance of the specified finger. */
    for ( i = 0; i < pStorageContext->numTemplates; i++ ) {
        pTemplateData = &pStorageContext->fpStore[i];
        if (NULL == subjectId ||
            (0 == memcmp(subjectId, pTemplateData->subjectId,
                         sizeof(DPFR_SUBJECT_ID)) &&
            ((fingerPosition == DPFR_FINGER_POSITION_UNDEFINED) ||
            (fingerPosition == pTemplateData->fingerPosition))))
        {
            // Specified finger found...
            pStorageContext->cursorLocation = i;
            break;
        }
    }
    if (i >= pStorageContext->numTemplates) {
        // Moved past the last record...
        return FR_WRN_EOF;
    }

    return status;
}

/*
 * cvFPSAMoveToTemplate
 *
 * IN   storageContext
 * IN   fingerPosition
 * IN   subjectId
 *
 * Description:
 * Routine to position the cursor to the specified finger
 * position for a given subject.
 *
 * Returns: FR_OK FR_ERR_INVALID_PARAM
 */
DPFR_RESULT cvFPSAMoveToTemplate(PVOID storageContext,
                                 DPFR_FINGER_POSITION fingerPosition)
{
    DPFR_RESULT status = FR_OK;
    PCV_FP_TEMPLATE_LIST pStorageContext =
                           (PCV_FP_TEMPLATE_LIST)storageContext;

    /* Validate the storage context pointer */
    if (!pStorageContext ||
         pStorageContext != pFPTemplateList)
    {
        return FR_ERR_INVALID_PARAM;
    }

    if (fingerPosition >= pStorageContext->numTemplates)
    {
        return FR_WRN_EOF;
    }

    /* Store the cursor location */
    pStorageContext->cursorLocation = fingerPosition;

    return status;
}

/*
 * cvFPSANextTemplate
 *
 * IN   storageContext
 *
 * Description:
 * This routine will move the cursor location to point to the next
 * template in the list of templates.
 *
 * Returns: FR_OK FR_WRN_EOF FR_ERR_INVALID_PARAM
 */
DPFR_RESULT cvFPSANextTemplate(PVOID storageContext)
{
    DPFR_RESULT status = FR_OK;
    PCV_FP_TEMPLATE_LIST pStorageContext =
                           (PCV_FP_TEMPLATE_LIST)storageContext;

    /* Validate the storage context pointer */
    if (!pStorageContext ||
         pStorageContext != pFPTemplateList)
    {
        return FR_ERR_INVALID_PARAM;
    }

    /* Test and set the cursor location to the next template in the list */
    if (pStorageContext->cursorLocation + 1 < pStorageContext->numTemplates)
    {
        pStorageContext->cursorLocation += 1;
    } else {
        return FR_WRN_EOF;
    }

    return status;
}

/*
 * cvFPSAGetCurrentFinger
 *
 * IN   storageContext
 * OUT  subjectId
 * OUT  pFingerPosition
 *
 * Description:
 * This routine returns the subject ID and finger position
 * from the current cursor location of the list of templates..
 *
 * Returns: FR_OK FR_ERR_INVALID_PARAM
 */
DPFR_RESULT cvFPSAGetCurrentFinger(PVOID storageContext,
                                   DPFR_SUBJECT_ID subjectId,
                                   DPFR_FINGER_POSITION *pFingerPosition)
{
    PCV_FP_TEMPLATE_STORE pTemplateData;
 	DPFR_RESULT status = FR_OK;
    PCV_FP_TEMPLATE_LIST pStorageContext =
                           (PCV_FP_TEMPLATE_LIST)storageContext;
        /* Validate the storage context pointer */
    if (!pStorageContext ||
         pStorageContext != pFPTemplateList)
    {
        return FR_ERR_INVALID_PARAM;
    }
	pTemplateData = &pStorageContext->fpStore[pStorageContext->cursorLocation];
    /* Handle the single raw template case. */
    if (pStorageContext->rawTemplateLength != 0)
    {
        DPFR_SUBJECT_ID subId = {0};
        memcpy(subjectId, subId, sizeof(DPFR_SUBJECT_ID));
        if (pFingerPosition)
        {
            *pFingerPosition = 0;
        }
        return status;
    }

    /* Copy the subject ID for the current cursor location */
    if (subjectId)
    {
        memcpy(subjectId, pTemplateData->subjectId, sizeof(DPFR_SUBJECT_ID));
    }

    /* Copy the fingerposition for current cursor location */
    if ( pFingerPosition ) {
        *pFingerPosition = pTemplateData->fingerPosition;
    }

    return status;
}

/*
 * cvFPSAGetCurrentTemplate
 *
 * IN   storageContext
 * OUT  phTemplate
 *
 * Description:
 * This API will return the handle to the template for the
 * current cursor location.
 *
 * Returns: FR_OK FR_ERR_INVALID_PARAM
 */
DPFR_RESULT cvFPSAGetCurrentTemplate(PVOID storageContext,
                                     DPFR_HANDLE hEngineContext,
                                     DPFR_HANDLE *phTemplate)
{
    PCV_FP_TEMPLATE_STORE pTemplateData;
    DPFR_RESULT status   = FR_OK;
    cv_status   cvStatus = CV_SUCCESS;
    PCV_FP_TEMPLATE_LIST pStorageContext = (PCV_FP_TEMPLATE_LIST)storageContext;
    cv_obj_properties        objProperties;
    cv_obj_value_fingerprint *pTemplate;
    cv_obj_hdr               *pObj;

    /* Validate the storage context pointer */
    if (!pStorageContext ||
         pStorageContext != pFPTemplateList)
    {
        return FR_ERR_INVALID_PARAM;
    }
    pTemplateData = &pStorageContext->fpStore[pStorageContext->cursorLocation];
    /* Handle the single raw template case. */
    if (pStorageContext->rawTemplateLength != 0)
    {
        status = DPFRImport(hEngineContext,
                        pStorageContext->rawTemplate,
                        pStorageContext->rawTemplateLength,
                        DPFR_DT_DP_TEMPLATE,
                        DPFR_PURPOSE_IDENTIFICATION,
                        phTemplate);
        return status;
    }
    /* Return FP template at current cursor location */
    if (phTemplate)
    {
        memset(&objProperties, 0, sizeof(cv_obj_properties));
        objProperties.session = (cv_session *)pStorageContext->cvHandle;
        objProperties.objHandle = pTemplateData->pFPData;

        /* Get the object from CV storage */
        cvStatus = cvGetObj(&objProperties, &pObj);
        if (cvStatus != CV_SUCCESS)
        {
            return FR_ERR_INVALID_HANDLE;
        }

        /* Get the object pointers */
        cvFindObjPtrs(&objProperties, pObj);
        pTemplate = (cv_obj_value_fingerprint *)objProperties.pObjValue;
        if (pTemplate->type != CV_FINGERPRINT_TEMPLATE)
        {
            /* Not a fingerprint object */
            return FR_ERR_INVALID_HANDLE;
        }

        cvUpdateObjCacheLRUFromHandle(objProperties.objHandle);
        status = DPFRImport(hEngineContext,
                        pTemplate->fp,
                        pTemplate->fpLen,
                        DPFR_DT_DP_TEMPLATE,
                        DPFR_PURPOSE_IDENTIFICATION,
                        phTemplate);
    }

    return status;
}

/*
 * cvFPSAHintCurrentCache
 *
 * IN   storageContext
 * IN   score
 *
 * Description:
 * This API is optional and is not implemented at present. 
 *
 * Returns: FR_OK FR_ERR_INVALID_PARAM
 */
DPFR_RESULT cvFPSAHintCurrentCache(PVOID storageContext,
                                   int score)
{
    DPFR_RESULT status = FR_OK;

    return status;
}

/*
 * cvFPSAUpdateCurrentTemplate
 *
 * IN   storageContext
 * IN   hTemplate
 *
 * Description:
 * This API updates the fingerprint object in the CV object
 * store by invoking cvPutObj function. It will update the
 * table template pointer with the updated template information.
 * This function is optional and is not implemented at present.
 *
 * Returns: FR_OK FR_ERR_INVALID_PARAM
 */
DPFR_RESULT cvFPSAUpdateCurrentTemplate(PVOID storageContext,
                                        DPFR_HANDLE hTemplate)
{
    DPFR_RESULT status = FR_OK;

    return status;
}

/*
 * cvFPSAFreeTemplatesList
 *
 * IN/OUT None
 *
 * Description:
 * This routine will free the template list allocated
 * by the cvFPSAInit routine.
 *
 * Returns: FR_OK
 */
DPFR_RESULT cvFPSAFreeTemplatesList(void)
{
    DPFR_RESULT status = FR_OK;

    if (pFPTemplateList) {
        if (pFPTemplateList->fpStore)
        {
            cv_free(pFPTemplateList->fpStore,
                    sizeof(CV_FP_TEMPLATE_STORE) * pFPTemplateList->numTemplates);
        }
        cv_free(pFPTemplateList, sizeof(CV_FP_TEMPLATE_LIST));
    }

    return status;
}

