/******************************************************************************
 *
 *  Copyright 2007
 *  Broadcom Corporation
 *  16215 Alton Parkway
 *  PO Box 57013
 *  Irvine CA 92619-7013
 *
 *****************************************************************************/
/* 
 * Broadcom Corporation Credential Vault
 */
/*
 * cvmanager.c:  Credential Vault object storage handler
 */
/*
 * Revision History:
 *
 * 01/11/07 DPA Created.
*/
#include "cvmain.h"
#include "console.h"

#undef HD_DEBUG
#define DEBUG_WO_HD_MIRRORING
#define DEBUG_WO_ANTI_REPLAY_CHECK	/* Until monotonic counter functions are available in HW */

bool 
flashHeapCallback(uint8_t *address)
{
	/* this is the callback for the flash heap enumerate routine.  it is called once for each allocated */
	/* block in the flash heap.  if that block isn't found in the CV structures, it is deallocated */
	uint32_t i;

	for (i=0;i<MAX_DIR_PAGE_0_ENTRIES;i++)
	{
		if (CV_DIR_PAGE_0->dir0Objs[i].pObj == address)
			/* entry found */
			return TRUE;
	}
	/* not found, s/b deallocated */
	return FALSE;
}

void
cvCheckFlashHeapConsistency(void)
{
	/* this routine checks to see if any flash heap memory is allocated that isn't referenced.  this could happen */
	/* if memory was allocated, but a power failure occurred before the corresponding directory entry could be */
	/* written to flash.  if such memory is found, it is deallocated */
	
	phx_fenumerate(flashHeapCallback);
}

cv_bool
cvIsDir0Consistent(void)
{
	/* this routine checks to see if any of the entries in dir 0 point to invalid (non-allocated) flash addresses */
	cv_bool isConsistent = TRUE;
	uint32_t i;

	for (i=0;i<MAX_DIR_PAGE_0_ENTRIES;i++)
	{
		if (CV_DIR_PAGE_0->dir0Objs[i].hObj != 0 && !phx_fis_address_valid(CV_DIR_PAGE_0->dir0Objs[i].pObj))
		{
			/* invalid entry found */
			isConsistent = FALSE;
			CV_DIR_PAGE_0->dir0Objs[i].hObj = 0;
		}
	}
	return isConsistent;
}

uint32_t
cvComputeObjLen(cv_obj_properties *objProperties)
{
	/* this routine uses the information the objProperties structure to return the object length */
	return (objProperties->authListsLength + objProperties->objAttributesLength + objProperties->objValueLength
		+ sizeof(cv_obj_hdr) + 2*sizeof(uint16_t) /* for attribute and auth list length fields */);
}

void *
cvFindObjValue(cv_obj_hdr *objPtr)
{
	/* this routine parses an object and returns the pointer to the object value field */
	cv_obj_attribs_hdr		*attribsHdr;
	cv_obj_auth_lists_hdr	*authListsHdr;

	attribsHdr = (cv_obj_attribs_hdr *)(objPtr + 1);
	authListsHdr = (cv_obj_auth_lists_hdr *)((uint8_t *)attribsHdr + attribsHdr->attribsLen + sizeof(uint16_t));
	return (uint8_t *)authListsHdr + authListsHdr->authListsLen + sizeof(uint16_t);
}

void
cvFindObjPtrs(cv_obj_properties *objProperties, cv_obj_hdr *objPtr)
{
	/* this routine parses an object and stores the pointers to fields in cv_obj_properties structure */
	uint16_t *lenPtr;

	lenPtr = (uint16_t *)(objPtr + 1);
	objProperties->objAttributesLength = *lenPtr;
	objProperties->pObjAttributes = (cv_obj_attributes *)(lenPtr + 1);
	lenPtr = (uint16_t *)(((uint8_t *)objProperties->pObjAttributes	+ objProperties->objAttributesLength));
	objProperties->authListsLength = *lenPtr;
	objProperties->pAuthLists = (cv_auth_list *)(lenPtr + 1);
	objProperties->objValueLength = objPtr->objLen - sizeof(cv_obj_hdr) - objProperties->objAttributesLength 
		- objProperties->authListsLength - 2*sizeof(uint16_t) /* 2 length fields */;
	objProperties->pObjValue = (uint8_t *)objProperties->pAuthLists + objProperties->authListsLength;
}

void
cvUpdateObjCacheMRU(uint32_t index) {
	uint32_t i, j;
	uint32_t MRUEntry;

	/* find index in MRU list */
	for (i = 0; i < MAX_CV_OBJ_CACHE_NUM_OBJS; i++) {
		if (CV_VOLATILE_DATA->objCacheMRU[i] == index) {
			/* move value at 'index' to top of MRU list */
			MRUEntry = CV_VOLATILE_DATA->objCacheMRU[i];
			for (j = i; j > 0; j--) 
				CV_VOLATILE_DATA->objCacheMRU[j] = CV_VOLATILE_DATA->objCacheMRU[j - 1]; // rearrange the elements
			CV_VOLATILE_DATA->objCacheMRU[0] = MRUEntry; // Set the start of the cache to the item we are interested in
			break; // Exit the loop if entry is found
		}
	}

	if (i == MAX_CV_OBJ_CACHE_NUM_OBJS) {
		printf("%s: Did not update the Object Cache MRU \n", __FUNCTION__);
	}
}

void
cvUpdateObjCacheMRUFromHandle(cv_obj_handle handle) 
{
	/* this routine finds the entry containing the given handle and makes it the MRU entry */
	uint32_t i;

	/* find handle in obj cache */
	for (i=0;i<MAX_CV_OBJ_CACHE_NUM_OBJS;i++)
		if (CV_VOLATILE_DATA->objCache[i] == handle)
			break;

	/* now update MRU entry if found */
	if (i<MAX_CV_OBJ_CACHE_NUM_OBJS)
		cvUpdateObjCacheMRU(i);
}

void
cvUpdateObjCacheLRU(uint32_t index) {
	uint32_t i, j;
	uint32_t LRUEntry;

	/* find index in MRU list */
	for (i = 0; i < MAX_CV_OBJ_CACHE_NUM_OBJS; i++) {
		if (CV_VOLATILE_DATA->objCacheMRU[i] == index) {
			/* move value at 'index' to bottom of MRU list */
			LRUEntry = CV_VOLATILE_DATA->objCacheMRU[i];
			for (j = i; j < (MAX_CV_OBJ_CACHE_NUM_OBJS - 1); j++) 
				CV_VOLATILE_DATA->objCacheMRU[j] = CV_VOLATILE_DATA->objCacheMRU[j + 1];
			CV_VOLATILE_DATA->objCacheMRU[MAX_CV_OBJ_CACHE_NUM_OBJS - 1] = LRUEntry;
			break;
		}
	}
	if (i == MAX_CV_OBJ_CACHE_NUM_OBJS) {
		printf("%s: Did not update the Object Cache LRU \n", __FUNCTION__);
	}
}

void
cvUpdateObjCacheLRUFromHandle(cv_obj_handle handle) 
{
	/* this routine finds the entry containing the given handle and makes it the LRU entry */
	uint32_t i;

	/* find handle in obj cache */
	for (i=0;i<MAX_CV_OBJ_CACHE_NUM_OBJS;i++)
		if (CV_VOLATILE_DATA->objCache[i] == handle)
			break;

	/* now update LRU entry */
	cvUpdateObjCacheLRU(i);
}

void
cvUpdateLevel1CacheMRU(uint32_t index)
{
	uint32_t i, j;
	uint32_t MRUEntry;

	/* find index in MRU list */
	for (i=0;i<MAX_CV_LEVEL1_DIR_CACHE_NUM_PAGES;i++)
	{
		if (CV_VOLATILE_DATA->level1DirCacheMRU[i] == index)
		{
			/* move value at 'index' to top of MRU list */
			MRUEntry = CV_VOLATILE_DATA->level1DirCacheMRU[i];
			for (j=i; j>0; j--)
				CV_VOLATILE_DATA->level1DirCacheMRU[j] = CV_VOLATILE_DATA->level1DirCacheMRU[j-1];
			CV_VOLATILE_DATA->level1DirCacheMRU[0] = MRUEntry;
			break;
		}
	}
	if(i == MAX_CV_LEVEL1_DIR_CACHE_NUM_PAGES)
	{
		printf("%s: Did not update the Level1 Cache MRU  \n",__FUNCTION__);
	}
}

void
cvUpdateLevel2CacheMRU(uint32_t index)
{
	uint32_t i, j;
	uint32_t MRUEntry;

	/* find index in MRU list */
	for (i=0;i<MAX_CV_LEVEL2_DIR_CACHE_NUM_PAGES;i++)
	{
		if (CV_VOLATILE_DATA->level2DirCacheMRU[i] == index)
		{
			/* move value at 'index' to top of MRU list */
			MRUEntry = CV_VOLATILE_DATA->level2DirCacheMRU[i];
			for (j=i; j>0; j--)
				CV_VOLATILE_DATA->level2DirCacheMRU[j] = CV_VOLATILE_DATA->level2DirCacheMRU[j-1];
			CV_VOLATILE_DATA->level2DirCacheMRU[0] = MRUEntry;
			break;
		}
	}
	if(i == MAX_CV_LEVEL2_DIR_CACHE_NUM_PAGES)
	{
		printf("%s: Did not update the Level2 Cache MRU  \n",__FUNCTION__);
	}

}

void
cvObjCacheGetLRU(uint32_t *LRUObj, uint8_t **objStoragePtr)
{
	/* not in object cache, need to read in.  get slot in object cache */
	/* get pointer to LRU object slot in object cache */
	*LRUObj = CV_VOLATILE_DATA->objCacheMRU[MAX_CV_OBJ_CACHE_NUM_OBJS-1];
	*objStoragePtr = &CV_OBJ_CACHE[*LRUObj*CV_OBJ_CACHE_ENTRY_SIZE];
	/* invalidate entry, in case object read not successful, but previous object corrupted */
	CV_VOLATILE_DATA->objCache[*LRUObj] = 0;
}

void
cvLevel1DirCacheGetLRU(uint32_t *LRUL1Dir, uint8_t **level1DirPagePtr)
{
	/* not in dir page cache, need to read in.  get slot in dir page cache */
	/* get pointer to LRU object slot in dir page cache */
	*LRUL1Dir = CV_VOLATILE_DATA->level1DirCacheMRU[MAX_CV_LEVEL1_DIR_CACHE_NUM_PAGES-1];
	*level1DirPagePtr = &CV_LEVEL1_DIR_PAGE_CACHE[*LRUL1Dir*sizeof(cv_dir_page_level1)];
	/* invalidate entry, in case object read not successful, but previous object corrupted */
	CV_VOLATILE_DATA->level1DirCache[*LRUL1Dir] = 0;
}

void
cvLevel2DirCacheGetLRU(uint32_t *LRUL2Dir, uint8_t **level2DirPagePtr)
{
	/* not in dir page cache, need to read in.  get slot in dir page cache */
	/* get pointer to LRU object slot in dir page cache */
	*LRUL2Dir = CV_VOLATILE_DATA->level2DirCacheMRU[MAX_CV_LEVEL2_DIR_CACHE_NUM_PAGES-1];
	*level2DirPagePtr = &CV_LEVEL2_DIR_PAGE_CACHE[*LRUL2Dir*sizeof(cv_dir_page_level2)];
	/* invalidate entry, in case object read not successful, but previous object corrupted */
	CV_VOLATILE_DATA->level2DirCache[*LRUL2Dir] = 0;
}

uint8_t cvGetLevel1DirPage(uint8_t level2DirPage)
{
	/* this routine gets the level 1 directory page, given the level 2 directory page.  this implements an object */
	/* storage hierarchy, where the top level directory contains 16 level 1 subdirectories, each of which contains */
	/* 16 subdirectories, which contain the object handles.  directory page 0 and 255 are reserved for objects not */
	/* stored on the host hard drive and the mirrored version of directory page 0, respectively */

	return level2DirPage>>4;
}

uint8_t cvGetLevel2DirPage(uint8_t level1DirPage, uint8_t dirIndex)
{
	/* this routine gets the level 2 directory page, given the level 1 directory page and index.  */
	/* this implements an object */
	/* storage hierarchy, where the top level directory contains 16 level 1 subdirectories, each of which contains */
	/* 16 subdirectories, which contain the object handles.  directory page 0 and 255 are reserved for objects not */
	/* stored on the host hard drive and the mirrored version of directory page 0, respectively */

	return ((level1DirPage<<4) | dirIndex);
}

uint32_t cvGetLevel1DirPageHandle(uint32_t level2DirPageHandle)
{
	/* this routine gets the level 1 directory page handle, given the level 2 directory page handle.  */
	/* this implements an object */
	/* storage hierarchy, where the top level directory contains 16 level 1 subdirectories, each of which contains */
	/* 16 subdirectories, which contain the object handles.  directory page 0 and 255 are reserved for objects not */
	/* stored on the host hard drive and the mirrored version of directory page 0, respectively */

	return ((level2DirPageHandle>>4) & ~HANDLE_VALUE_MASK);
}

uint32_t cvGetLevel2FromL1DirPageHandle(uint32_t level1DirPage, uint32_t dirIndex)
{
	/* this routine gets the level 2 directory page handle, given the level 1 directory page handle.  */
	/* this implements an object */
	/* storage hierarchy, where the top level directory contains 16 level 1 subdirectories, each of which contains */
	/* 16 subdirectories, which contain the object handles.  directory page 0 and 255 are reserved for objects not */
	/* stored on the host hard drive and the mirrored version of directory page 0, respectively */

	return ((level1DirPage<<4) | (dirIndex<<HANDLE_DIR_SHIFT));
}

cv_bool
cvIsPageFull(uint32_t handle)
{
	uint8_t byteIndex, bitIndex, dirPage;

	dirPage = GET_DIR_PAGE(handle);
	byteIndex = dirPage/8;
	bitIndex = dirPage & 0x7;
	return CV_TOP_LEVEL_DIR->pageMapFull[byteIndex] & (0x1<<bitIndex);
}

void
cvSetPageFull(uint32_t handle)
{
	uint8_t byteIndex, bitIndex, dirPage;

	dirPage = GET_DIR_PAGE(handle);
	byteIndex = dirPage/8;
	bitIndex = dirPage & 0x7;
	CV_TOP_LEVEL_DIR->pageMapFull[byteIndex] |= (0x1<<bitIndex);
}

void
cvSetPageAvail(uint32_t handle)
{
	uint8_t byteIndex, bitIndex, dirPage;

	dirPage = GET_DIR_PAGE(handle);
	byteIndex = dirPage/8;
	bitIndex = dirPage & 0x7;
	CV_TOP_LEVEL_DIR->pageMapFull[byteIndex] &= ~(0x1<<bitIndex);
}

cv_bool
cvPageExists(uint32_t handle)
{
	uint8_t byteIndex, bitIndex, dirPage;

	dirPage = GET_DIR_PAGE(handle);
	byteIndex = dirPage/8;
	bitIndex = dirPage & 0x7;
	return CV_TOP_LEVEL_DIR->pageMapExists[byteIndex] & (0x1<<bitIndex);
}

void
cvSetPageExists(uint32_t handle)
{
	uint8_t byteIndex, bitIndex, dirPage;

	dirPage = GET_DIR_PAGE(handle);
	byteIndex = dirPage/8;
	bitIndex = dirPage & 0x7;
	CV_TOP_LEVEL_DIR->pageMapExists[byteIndex] |= (0x1<<bitIndex);
}

void
cvSetPageAbsent(uint32_t handle)
{
	uint8_t byteIndex, bitIndex, dirPage;

	dirPage = GET_DIR_PAGE(handle);
	byteIndex = dirPage/8;
	bitIndex = dirPage & 0x7;
	CV_TOP_LEVEL_DIR->pageMapExists[byteIndex] &= ~(0x1<<bitIndex);
}

void
cvRemoveL2Entry(cv_obj_handle objHandle, cv_dir_page_level2 *dirL2Ptr)
{
	uint32_t i;

	for (i=0;i<MAX_CV_ENTRIES_PER_DIR_PAGE;i++)
	{
		if (dirL2Ptr->dirEntries[i].hObj == (objHandle | dirL2Ptr->header.thisDirPage.hDirPage))
		{
			dirL2Ptr->dirEntries[i].hObj = 0;
			cvSetPageAvail(dirL2Ptr->header.thisDirPage.hDirPage);
			break;
		}
	}
}

void
cvCreateL1Dir(cv_obj_handle level1DirHandle, cv_obj_handle level2DirHandle, 
			  uint32_t *LRUL1Dir, cv_dir_page_level1 **dirL1Ptr)
{
	cv_top_level_dir *topLevelDir = CV_TOP_LEVEL_DIR;
	uint32_t entry = GET_DIR_PAGE(level1DirHandle);

	topLevelDir->dirEntries[entry].hDirPage = level1DirHandle;
	cvLevel1DirCacheGetLRU(LRUL1Dir, (uint8_t **)dirL1Ptr);
	memset(*dirL1Ptr,0,sizeof(cv_dir_page_level1));
	(*dirL1Ptr)->header.thisDirPage.hDirPage = level1DirHandle;
	(*dirL1Ptr)->dirEntries[0].hDirPage = level2DirHandle;
}

void
cvCreateL2Dir(cv_obj_storage_attributes objAttribs, cv_obj_properties *objProperties, 
			  cv_obj_handle *objHandle, cv_dir_page_level1 *dirL1Ptr, uint32_t l1Index, 
			  uint32_t *LRUL2Dir, cv_dir_page_level2 **dirL2Ptr)
{
	uint32_t l2Index = 0;

	/* if page not already in cache, get cache entry and create directory page */
	if (*dirL2Ptr == NULL)
	{
		dirL1Ptr->dirEntries[l1Index].hDirPage = cvGetLevel2FromL1DirPageHandle(dirL1Ptr->header.thisDirPage.hDirPage, l1Index);
		cvLevel2DirCacheGetLRU(LRUL2Dir, (uint8_t **)dirL2Ptr);
		memset(*dirL2Ptr,0,sizeof(cv_dir_page_level2));
		(*dirL2Ptr)->header.thisDirPage.hDirPage = dirL1Ptr->dirEntries[l1Index].hDirPage;
	} else {
		/* page already in cache, get available entry */
		l2Index = cvGetAvailL2Index(*dirL2Ptr);
		memset(&(*dirL2Ptr)->dirEntries[l2Index],0,sizeof(cv_dir_page_entry_obj));
	}
	memcpy(&((*dirL2Ptr)->dirEntries[l2Index].appUserID), &objProperties->session->appUserID, 
		sizeof(objProperties->session->appUserID));
	(*dirL2Ptr)->dirEntries[l2Index].attributes = objAttribs;
	*objHandle |= (*dirL2Ptr)->header.thisDirPage.hDirPage;
	(*dirL2Ptr)->dirEntries[l2Index].hObj = *objHandle;
}

cv_status
cvGetL1L2Dir(cv_obj_properties *objProperties, cv_obj_handle level1DirHandle, cv_obj_handle level2DirHandle, 
		   uint32_t *LRUL1Dir, cv_dir_page_level1 **dirL1Ptr, uint32_t *LRUL2Dir, cv_dir_page_level2 **dirL2Ptr)		   
{
	cv_status retVal = CV_SUCCESS;
	cv_obj_handle objDirHandles[3];
	uint8_t *objDirPtrs[3], *objDirPtrsOut[3];
	cv_obj_storage_attributes objAttribs;

	if (dirL1Ptr != NULL)
		*dirL1Ptr = NULL;
	if (dirL2Ptr != NULL)
		*dirL2Ptr = NULL;
	/* see if L1 page is requested and exists.  if so, retrieve it */
	if (level1DirHandle && cvPageExists(level1DirHandle) && dirL1Ptr != NULL)
	{
		cvLevel1DirCacheGetLRU(LRUL1Dir, (uint8_t **)dirL1Ptr);
		objDirHandles[0] = level1DirHandle;
		objDirPtrs[0] = (uint8_t *)(*dirL1Ptr);
	} else {
		objDirHandles[0] = 0;
		objDirPtrs[0] = 0;
	}
	/* see if L2 page is requested and exists.  if so, retrieve it too */
	if (level2DirHandle && cvPageExists(level2DirHandle) && dirL2Ptr != NULL)
	{
		cvLevel2DirCacheGetLRU(LRUL2Dir, (uint8_t **)dirL2Ptr);
		objDirHandles[1] = level2DirHandle;
		objDirPtrs[1] = (uint8_t *)(*dirL2Ptr);
	} else {
		objDirHandles[1] = 0;
		objDirPtrs[1] = 0;
	}
	objDirHandles[2] = 0;
	objDirPtrs[2] = 0;
	if (objDirHandles[0] || objDirHandles[1])
	{
		if ((retVal = cvGetHostObj(objDirHandles, objDirPtrs, objDirPtrsOut)) != CV_SUCCESS)
			goto err_exit;
		/* now unwrap object */
		objAttribs.storageType = CV_STORAGE_TYPE_HARD_DRIVE;
		if ((retVal = cvUnwrapObj(objProperties, objAttribs, objDirHandles, objDirPtrs, objDirPtrsOut)) != CV_SUCCESS)
			goto err_exit;
	}
err_exit:
	return retVal;
}

void
cvFlushCacheEntry(cv_obj_handle objHandle)
{
	uint32_t i;

	for (i=0;i<MAX_CV_OBJ_CACHE_NUM_OBJS;i++)
	{
		if (CV_VOLATILE_DATA->objCache[i] == objHandle)
		{
			/* object found in cache.  flush entry */
			CV_VOLATILE_DATA->objCache[i] = 0;
			break;
		}
	}
}

void
cvFlushAllCaches(void)
{
	/* this routine will initialize the object and directory page caches */
	uint32_t i;

	memset(CV_VOLATILE_DATA->objCache,0,sizeof(cv_obj_handle)*MAX_CV_OBJ_CACHE_NUM_OBJS);
	memset(CV_VOLATILE_DATA->level1DirCache,0,sizeof(cv_obj_handle)*MAX_CV_LEVEL1_DIR_CACHE_NUM_PAGES);
	memset(CV_VOLATILE_DATA->level2DirCache,0,sizeof(cv_obj_handle)*MAX_CV_LEVEL2_DIR_CACHE_NUM_PAGES);
	for (i=0;i<MAX_CV_OBJ_CACHE_NUM_OBJS;i++)
		CV_VOLATILE_DATA->objCacheMRU[i] = i;
	for (i=0;i<MAX_CV_LEVEL1_DIR_CACHE_NUM_PAGES;i++)
		CV_VOLATILE_DATA->level1DirCacheMRU[i] = i;
	for (i=0;i<MAX_CV_LEVEL2_DIR_CACHE_NUM_PAGES;i++)
		CV_VOLATILE_DATA->level2DirCacheMRU[i] = i;
}

cv_bool
cvFindObjInCache(cv_obj_handle objHandle, cv_obj_hdr **objPtr, uint32_t *MRUIndex)
{
	uint32_t i;

	for (i=0;i<MAX_CV_OBJ_CACHE_NUM_OBJS;i++)
	{
		if (CV_VOLATILE_DATA->objCache[i] == objHandle)
		{
			/* object found in cache return pointer */
			*objPtr = (cv_obj_hdr *)&CV_OBJ_CACHE[CV_OBJ_CACHE_ENTRY_SIZE*i];
			*MRUIndex = i;
			return TRUE;
		}
	}
	return FALSE;
}

cv_bool
cvFindDirPageInL1Cache(cv_obj_handle objHandle, cv_dir_page_level1 **dirL1Ptr, uint32_t *MRUIndex)
{
	cv_obj_handle level1DirHandle;
	uint32_t i;

	level1DirHandle = GET_L1_HANDLE_FROM_OBJ_HANDLE(objHandle);
	for (i=0;i<MAX_CV_LEVEL1_DIR_CACHE_NUM_PAGES;i++) {
		if (CV_VOLATILE_DATA->level1DirCache[i] == level1DirHandle) 
		{
			*dirL1Ptr = (cv_dir_page_level1 *)&CV_LEVEL1_DIR_PAGE_CACHE[sizeof(cv_dir_page_level1)*i];
			*MRUIndex = i;
			return TRUE;
		}
	}
	return FALSE;
}

cv_bool
cvFindDirPageInL2Cache(cv_obj_handle objHandle, cv_dir_page_level2 **dirL2Ptr, uint32_t *MRUIndex)
{
	uint32_t i;
	cv_obj_handle level2DirHandle;

	if (objHandle)
		/* looking for a specific entry */
		level2DirHandle = GET_L2_HANDLE_FROM_OBJ_HANDLE(objHandle);
	else
		/* looking for any entry */
		level2DirHandle = 0;
	for (i=0;i<MAX_CV_LEVEL2_DIR_CACHE_NUM_PAGES;i++)
	{
		if ((level2DirHandle && CV_VOLATILE_DATA->level2DirCache[i] == level2DirHandle)
			|| (!level2DirHandle && CV_VOLATILE_DATA->level2DirCache[i] !=0))
		{
			/* directory found in cache, check to see if entry available */
			*dirL2Ptr = (cv_dir_page_level2 *)&CV_LEVEL2_DIR_PAGE_CACHE[sizeof(cv_dir_page_level2)*i];
			/* if looking for available entry, test to make sure this dir page isn't full */
			if (!level2DirHandle && cvIsPageFull((*dirL2Ptr)->header.thisDirPage.hDirPage))
				continue;
			*MRUIndex = i;
			return TRUE;
		}
	}
	return FALSE;
}

cv_bool
cvFindDirPageInPageMap(cv_obj_handle *level2DirHandle)
{
	uint32_t i;

	for (i=MIN_L2_DIR_PAGE;i<=MAX_L2_DIR_PAGE;i++)
	{
		*level2DirHandle = GET_HANDLE_FROM_PAGE(i);
		if (!cvIsPageFull(*level2DirHandle))
			return TRUE;
	}
	return FALSE;
}

cv_bool
cvFindDir0Entry(cv_obj_handle handle, uint32_t *dir0Index)
{
	uint32_t i;

	for (i=0;i<MAX_DIR_PAGE_0_ENTRIES;i++)
	{
		if (CV_DIR_PAGE_0->dir0Objs[i].hObj == handle)
		{
			/* entry found */
			*dir0Index = i;
			return TRUE;
		}
	}
	return FALSE;
}

cv_status
cvCreateDir0Entry(cv_obj_storage_attributes objAttribs, cv_obj_properties *objProperties, 
				  cv_obj_hdr *objPtr, uint8_t **objDirPtrs, cv_obj_handle objHandle, uint32_t *index)
{
	uint32_t dir0Index;

	/* first search for available entry */
	if (!cvFindDir0Entry(0, &dir0Index))
		return CV_NO_PERSISTENT_OBJECT_ENTRY_AVAIL;
	*index = dir0Index;
	memset(&CV_DIR_PAGE_0->dir0Objs[dir0Index],0,sizeof(cv_dir_page_entry_dir));
	CV_DIR_PAGE_0->dir0Objs[dir0Index].pObj = objDirPtrs[2];
	memcpy(&CV_DIR_PAGE_0->dir0Objs[dir0Index].appUserID, &objProperties->session->appUserID, 
		sizeof(objProperties->session->appUserID));
	CV_DIR_PAGE_0->dir0Objs[dir0Index].hObj = objHandle;
	CV_DIR_PAGE_0->dir0Objs[dir0Index].objLen = cvGetWrappedSize(objPtr->objLen);
	//CV_DIR_PAGE_0->dir0Objs[dir0Index].objLen = objPtr->objLen;
	CV_DIR_PAGE_0->dir0Objs[dir0Index].attributes = objAttribs;
	return CV_SUCCESS;
}

cv_bool
cvFindVolatileDirEntry(cv_obj_handle objHandle, uint32_t *index)
{
	uint32_t volDirIndex;

	for (volDirIndex=0;volDirIndex<MAX_CV_NUM_VOLATILE_OBJS;volDirIndex++)
	{
		if (CV_VOLATILE_OBJ_DIR->volObjs[volDirIndex].hObj == objHandle)
		{
			*index = volDirIndex;
			return TRUE;
		}
	}
	return FALSE;
}

cv_status
cvCreateVolatileDirEntry(cv_obj_storage_attributes objAttribs, cv_obj_properties *objProperties, 
				  cv_obj_hdr *objPtr, cv_obj_handle objHandle)
{
	uint32_t volDirIndex;
	uint8_t *buf;
	cv_session *session = NULL;

	if (objProperties != NULL)
		session = objProperties->session;
	/* see if space can be allocated for object */
	if ((buf = cv_malloc(objPtr->objLen)) == NULL)
		return CV_VOLATILE_MEMORY_ALLOCATION_FAIL;
	if (cvFindVolatileDirEntry(0,&volDirIndex))
	{
		/* entry found, use it */
		CV_VOLATILE_OBJ_DIR->volObjs[volDirIndex].pObj = buf;
		CV_VOLATILE_OBJ_DIR->volObjs[volDirIndex].hObj = objHandle;
		CV_VOLATILE_OBJ_DIR->volObjs[volDirIndex].hSession = objProperties->session;
		/* check to see if object should be retained after session closed.  if so, don't save session handle */
		if (session != NULL && (session->flags & CV_RETAIN_VOLATILE_OBJS))
			CV_VOLATILE_OBJ_DIR->volObjs[volDirIndex].hSession = 0;
		memcpy(&CV_VOLATILE_OBJ_DIR->volObjs[volDirIndex].appUserID, &objProperties->session->appUserID, 
			sizeof(objProperties->session->appUserID));
		/* compose object in volatile memory */
		cvComposeObj(objProperties, objPtr, buf);
	} else {
		cv_free(buf, objPtr->objLen);
		return CV_NO_VOLATILE_OBJECT_ENTRY_AVAIL;
	}
	objProperties->objHandle = objHandle;
	return CV_SUCCESS;
}

cv_status
cvUpdateVolatileDirEntry(cv_obj_storage_attributes objAttribs, cv_obj_properties *objProperties, 
				  cv_obj_hdr *objPtr, cv_obj_handle objHandle)
{
	uint32_t volDirIndex;
	uint8_t *buf;
	cv_obj_hdr *existingObj;

	if (cvFindVolatileDirEntry(objHandle,&volDirIndex))
	{
		/* entry found, use it */
		/* check to see if object will fit in space allocated */
		existingObj = (cv_obj_hdr *)CV_VOLATILE_OBJ_DIR->volObjs[volDirIndex].pObj;
		buf = (uint8_t *)existingObj;
		if (existingObj->objLen < objPtr->objLen)
		{
			/* new obj is larger, need to realloc */
			if ((buf = cv_malloc(objPtr->objLen)) == NULL)
				return CV_VOLATILE_MEMORY_ALLOCATION_FAIL;
			cv_free(existingObj, existingObj->objLen);
			/* update directory pointer */
			CV_VOLATILE_OBJ_DIR->volObjs[volDirIndex].pObj = buf;
		}
		/* compose object in volatile memory */
		cvComposeObj(objProperties, objPtr, buf);
	} else {
		return CV_INVALID_OBJECT_HANDLE;
	}
	return CV_SUCCESS;
}

cv_bool
cvFindObjEntryInL2Dir(cv_obj_handle objHandle, cv_dir_page_level2 *dirL2Ptr, uint32_t *index)
{
	uint32_t i;

	for (i=0;i<MAX_CV_ENTRIES_PER_DIR_PAGE;i++)
	{
		if (objHandle == dirL2Ptr->dirEntries[i].hObj)
		{
			*index = i;
			return TRUE;
		}
	}
	return FALSE;
}

cv_bool
cvFindL2DirEntryInL1Dir(cv_obj_handle dirHandle, cv_dir_page_level1 *dirL1Ptr, uint32_t *index)
{
	uint32_t i;

	for (i=0;i<MAX_CV_ENTRIES_PER_DIR_PAGE;i++)
	{
		if (dirHandle == dirL1Ptr->dirEntries[i].hDirPage)
		{
			*index = i;
			return TRUE;
		}
	}
	return FALSE;
}

cv_bool
cvFindL1DirEntryInTopLevelDir(cv_obj_handle dirHandle, uint32_t *index)
{
	cv_top_level_dir *topLevelDir = CV_TOP_LEVEL_DIR;
	uint32_t i;

	for (i=0;i<MAX_CV_ENTRIES_PER_DIR_PAGE;i++)
	{
		if (dirHandle == topLevelDir->dirEntries[i].hDirPage)
		{
			*index = i;
			return TRUE;
		}
	}
	return FALSE;
}

void
cvComposeObj(cv_obj_properties *objProperties, cv_obj_hdr *objPtr, uint8_t *buf)
{
	/* this routine composes an object from component parts */
	uint8_t *bufPtr = buf;
	uint16_t *lenPtr;

	memcpy(bufPtr, objPtr, sizeof(cv_obj_hdr));
	bufPtr += sizeof(cv_obj_hdr);
	lenPtr = (uint16_t *)bufPtr;
	*lenPtr = objProperties->objAttributesLength;
	bufPtr = (uint8_t *)(lenPtr + 1);
	memcpy(bufPtr, objProperties->pObjAttributes, objProperties->objAttributesLength);
	lenPtr = (uint16_t *)(bufPtr + objProperties->objAttributesLength);
	*lenPtr = objProperties->authListsLength;
	bufPtr = (uint8_t *)(lenPtr + 1);
	memcpy(bufPtr, objProperties->pAuthLists, objProperties->authListsLength);
	bufPtr += objProperties->authListsLength;
	memcpy(bufPtr, objProperties->pObjValue, objProperties->objValueLength);
}

cv_bool
cvIsL2DirPageFull(cv_dir_page_level2 *l2Ptr)
{
	uint16_t i;

	for (i=0;i<MAX_CV_ENTRIES_PER_DIR_PAGE;i++)
		if (l2Ptr->dirEntries[i].hObj == 0)
			return FALSE;
	return TRUE;
}

uint16_t
cvGetAvailL2Index(cv_dir_page_level2 *l2Ptr)
{
	uint16_t i;

	for (i=0;i<MAX_CV_ENTRIES_PER_DIR_PAGE;i++)
		if (l2Ptr->dirEntries[i].hObj == 0)
			break;
	return i;
}

uint16_t
cvGetAvailL1Index(cv_dir_page_level1 *l1Ptr)
{
	uint16_t i;

	for (i=0;i<MAX_CV_ENTRIES_PER_DIR_PAGE;i++)
		if ((l1Ptr->dirEntries[i].hDirPage != 0 && !cvIsPageFull(l1Ptr->dirEntries[i].hDirPage))
			|| l1Ptr->dirEntries[i].hDirPage == 0)
			break;
	return i;
}

uint32_t cvGetWrappedSize(uint32_t objLen)
{
	/* this routine gets the size of a object after encryption wrapper added */
	uint32_t hmacLen, padLen;

	/* add HMAC len and padding */
	hmacLen = SHA256_LEN;
	padLen = GET_AES_128_PAD_LEN(objLen + hmacLen + sizeof(cv_obj_dir_page_blob_enc));
	return (objLen + hmacLen + padLen + sizeof(cv_obj_dir_page_blob));
}

cv_status
cvDetermineObjAvail(cv_obj_properties *objProperties)
{
	/* this function determines if an object can be retrieved, or if prompting required */
	cv_status retVal = CV_SUCCESS;
	cv_obj_handle objHandle = objProperties->objHandle;
	uint32_t i;
	cv_bool objFound = FALSE;

	/* first search for the object in the object cache */
	for (i=0;i<MAX_CV_OBJ_CACHE_NUM_OBJS;i++)
		if (CV_VOLATILE_DATA->objCache[i] == objHandle)
			goto obj_found;
	/* check to see if object is in directory page 0 */
	if (GET_DIR_PAGE(objHandle) == 0)
	{
		/* yes, first search volatile object directory */
		for (i=0;i<MAX_CV_NUM_VOLATILE_OBJS;i++)
			if (CV_VOLATILE_OBJ_DIR->volObjs[i].hObj == objHandle)
				goto obj_found;
		/* now search directory page 0 */
		for (i=0;i<MAX_DIR_PAGE_0_ENTRIES;i++)
		{
			if (CV_DIR_PAGE_0->dir0Objs[i].hObj == objHandle)
			{
				/* object found, determine where object resident */
				switch (CV_DIR_PAGE_0->dir0Objs[i].attributes.storageType)
				{
				case CV_STORAGE_TYPE_FLASH:
					goto obj_found;
				case CV_STORAGE_TYPE_SMART_CARD:
					/* check to see if smart card inserted */
					if (scIsCardPresent())
					{
						/* smart card present, check to see if PIN required */
						if (CV_DIR_PAGE_0->dir0Objs[i].attributes.PINRequired)
						{
							/* yes, return PIN required status */
							retVal = CV_PROMPT_PIN;
							goto err_exit;
						} else {
							/* no PIN required, return success */
							goto obj_found;
						}
					} else {
						/* smart card not present, see if PIN also required */
						if (CV_DIR_PAGE_0->dir0Objs[i].attributes.PINRequired)
						{
							/* yes, return insertion and PIN required status */
							retVal = CV_PROMPT_PIN_AND_SMART_CARD;
							goto err_exit;
						} else {
							/* only prompt for smart card */
							retVal = CV_PROMPT_FOR_SMART_CARD;
							goto err_exit;
						}
					}
				case CV_STORAGE_TYPE_CONTACTLESS:
					/* check to see if contactless inserted */
					if (CV_VOLATILE_DATA->CVDeviceState & CV_CONTACTLESS_SC_PRESENT)
					{
						/* contactless present, check to see if PIN required */
						if (CV_DIR_PAGE_0->dir0Objs[i].attributes.PINRequired)
						{
							/* yes, return PIN required status */
							retVal = CV_PROMPT_PIN;
							goto err_exit;
						} else {
							/* no PIN required, return success */
							goto obj_found;
						}
					} else {
						/* contactless not present, see if PIN also required */
						if (CV_DIR_PAGE_0->dir0Objs[i].attributes.PINRequired)
						{
							/* yes, return insertion and PIN required status */
							retVal = CV_PROMPT_PIN_AND_CONTACTLESS;
							goto err_exit;
						} else {
							/* only prompt for contactless */
							retVal = CV_PROMPT_FOR_CONTACTLESS;
							goto err_exit;
						}
					}
				case 0xffff:
					break;
				}
				objFound = TRUE;
				break;
			}
		}
		/* if object not found, error because handle indicated object s/b in directory 0 */
		if (!objFound)
		{
			retVal = CV_INVALID_OBJECT_HANDLE;
			goto err_exit;
		}
	}
	/* here if object on host hard drive.  return sucess because object can be retrieved when needed */
err_exit:
obj_found:
	return retVal;
}

cv_status
cvWrapObj(cv_obj_properties *objProperties, cv_obj_storage_attributes objAttribs, cv_obj_handle *handles, 
		  uint8_t **ptrs, uint32_t *objLen)
{
	/* this routine gets latest anti-replay count, authenticates and encrypts for object/directory page blobs */
	/* the handles list contains the object handle and if necessary, the handles for the directories */
	/* above the object in the hierarchy.  The objects and directories are pointed to by the corresponding entry */
	/* in the 'ptrs' list and the output is built in the command io buf */
	uint32_t hmacLen, cryptLen, authLen;
	cv_session *session = NULL;
	cv_obj_dir_page_blob *objBlob;
	cv_hmac_key hmacKey;
	uint8_t *AESKey2 = NULL, *AESKey1;
	uint8_t *pHMAC;
	cv_status retVal = CV_SUCCESS;
	uint32_t i;
	cv_dir_page_level1 *dirL1Ptr;
	cv_dir_page_level2 *dirL2Ptr;
	cv_host_storage_store_object *objStore = (cv_host_storage_store_object *)CV_SECURE_IO_COMMAND_BUF;
	uint8_t *buf, *bufStart;
	uint32_t count;
	cv_host_storage_object_entry *objDirEntry = NULL; /* compiler thinks this is used before set, but I don't think so */
	uint32_t padLen;
	cv_version version;
	uint8_t hmacKeyValue[14];

	if (objProperties != NULL)
		session = objProperties->session;
	/* first set up object blob header in command buf */
	/* if host destination, need to add transport header too */
	cvGetCurCmdVersion(&version);
	bufStart = buf = CV_SECURE_IO_COMMAND_BUF;
	/* for AH credit data, HDOTP skip table and TLD, must use alternate buffer, since CV_SECURE_IO_COMMAND_BUF may be in use */
	if (handles[2] == CV_ANTIHAMMERING_CREDIT_DATA_HANDLE || handles[2] == CV_HDOTP_TABLE_HANDLE 
		|| handles[2] == CV_TOP_LEVEL_DIR_HANDLE)
	{
	    if(objProperties == NULL)
	    {
			retVal = CV_OBJ_AUTH_FAIL;
			goto err_exit;
	    }
		bufStart = buf = objProperties->altBuf;
	}
	if ((retVal = get_predicted_monotonic_counter(&count)) != CV_SUCCESS)
		goto err_exit;
	if (objAttribs.storageType == CV_STORAGE_TYPE_HARD_DRIVE || objAttribs.storageType == CV_STORAGE_TYPE_HARD_DRIVE_NON_OBJ
		|| objAttribs.storageType == CV_STORAGE_TYPE_HARD_DRIVE_UCK)
	{
		objStore->transportType = CV_TRANS_TYPE_HOST_STORAGE;
		objStore->commandID = CV_HOST_STORAGE_STORE_OBJECT;
		buf += (sizeof(cv_host_storage_store_object));
		/* CommandID is 32-bits in USB transport, pad with zeroes */
		*buf++ = 0;
		*buf++ = 0;
		/* set up object entry, if any */
		if (handles[2])
		{
			objDirEntry = (cv_host_storage_object_entry *)buf;
			objDirEntry->handle = handles[2];
			buf += sizeof(cv_host_storage_object_entry);
		}
	}
	hmacLen = SHA256_LEN;

	/* compute HMAC key */
	memcpy(hmacKeyValue, "CV object blob", sizeof(hmacKeyValue));
	if ((cvAuth(hmacKeyValue, sizeof(hmacKeyValue), NULL, (uint8_t *)&hmacKey, hmacLen, NULL, 0, CV_SHA)) != CV_SUCCESS)
	{
		retVal = CV_OBJ_AUTH_FAIL;
		goto err_exit;
	}

	/* set up and wrap object, if any */
	/* Since DIR_PAGE0_HANDLE is defined as 0x0, check storage type as well */
	if (handles[2] || (objAttribs.storageType == CV_STORAGE_TYPE_FLASH_NON_OBJ))
	{
		objBlob = (cv_obj_dir_page_blob *)buf;
		objBlob->version.versionMajor = version.versionMajor;
		objBlob->version.versionMinor = version.versionMinor;
		objBlob->encArea.hObj = handles[2];
		objBlob->encArea.counter = count;
		objBlob->objPageDirLen = *objLen;
		memset(objBlob->IVPrivKey, 0, AES_128_IV_LEN);

		/* copy object to buffer */
		memcpy((byte *)(&objBlob->encArea + 1), ptrs[2], *objLen);

		cryptLen = objBlob->objPageDirLen + sizeof(cv_obj_dir_page_blob_enc) + hmacLen;
		/* see if padding required */
		padLen = GET_AES_128_PAD_LEN(cryptLen);
		memset((uint8_t *)&objBlob->encArea + cryptLen, 0, padLen);
		cryptLen += padLen;
		objBlob->encAreaLen = cryptLen;
		/* compute auth length and pointer to HMAC */
		authLen = sizeof(cv_obj_dir_page_blob) + *objLen;
		pHMAC = buf + authLen;
		/* check to see if privacy key being used by this session */
		if (session != NULL && (session->flags & CV_HAS_PRIVACY_KEY))
			AESKey2 = session->privacyKey;
		/* check for special cases for flash keys */
		if (objAttribs.storageType == CV_STORAGE_TYPE_FLASH_NON_OBJ || objAttribs.storageType == CV_STORAGE_TYPE_HARD_DRIVE_NON_OBJ)
			/* if not an object, don't use privacy key */
			AESKey2 = NULL;
		/* if importing object which already has privacy encryption, don't use privacy key, but use imported IV for blob */
		if (objAttribs.hasPrivacyKey)
		{
			AESKey2 = NULL;
			if (objProperties != NULL)
				memcpy(objBlob->IVPrivKey, objProperties->pIVPriv, AES_128_IV_LEN);
		}
		if (objAttribs.storageType == CV_STORAGE_TYPE_FLASH_PERSIST_DATA)
		{
			/* use 6T flash key to encrypt persistent data */
			AESKey1 = NV_6TOTP_DATA->Kaes;
			AESKey2 = NULL;
		} else if (objAttribs.storageType == CV_STORAGE_TYPE_HARD_DRIVE_UCK)
			/* if UCK, encrypt with GCK */
			AESKey1 = CV_PERSISTENT_DATA->GCKKey;
		else
			AESKey1 = CV_PERSISTENT_DATA->UCKKey;
		/* now do encryption and auth.  privacy key, if any, only encrypts object itself (not including object header) */
		if ((retVal = cvEncryptBlob(buf, authLen, (uint8_t *)&objBlob->encArea, cryptLen,
			(uint8_t *)(&objBlob->encArea + 1) + sizeof(cv_obj_hdr), objBlob->objPageDirLen - sizeof(cv_obj_hdr),
			objBlob->IV, AESKey1, objBlob->IVPrivKey, AESKey2, (uint8_t *)&hmacKey, pHMAC, hmacLen, CV_CBC,
			NULL, 0)) != CV_SUCCESS)
				goto err_exit;
		*objLen = cryptLen + sizeof(cv_obj_dir_page_blob) - sizeof(cv_obj_dir_page_blob_enc);
		buf += *objLen;
	}

	/* for host hard drive storage, need to wrap directory pages too (unless this is a mirrored object) */
	if (objAttribs.storageType == CV_STORAGE_TYPE_HARD_DRIVE && ((handles[2] & CV_OBJ_HANDLE_PAGE_MASK) != CV_MIRRORED_OBJ_MASK))
	{
		/* set up length for object entry (if object was included) */
		if (handles[2])
			objDirEntry->length = *objLen;
		/* now set up and encrypt directory pages */
		dirL1Ptr = (cv_dir_page_level1 *)ptrs[0];
		dirL2Ptr = (cv_dir_page_level2 *)ptrs[1];
		/* level 2 */
		objDirEntry = (cv_host_storage_object_entry *)buf;
		objDirEntry->handle = handles[1];
		buf = (uint8_t *)(objDirEntry + 1);
		objBlob = (cv_obj_dir_page_blob *)buf;
		cvGetCurCmdVersion(&version);
		objBlob->version.versionMajor = version.versionMajor;
		objBlob->version.versionMinor = version.versionMinor;
		objBlob->encArea.hObj = handles[1];
		objBlob->encArea.counter = count;
		objBlob->objPageDirLen = sizeof(cv_dir_page_level2);
		dirL2Ptr->header.thisDirPage.counter = count;
		if (!cvFindObjEntryInL2Dir(handles[2], dirL2Ptr, &i))
		{
			retVal = CV_INVALID_HANDLE;
			goto err_exit;
		}
		dirL2Ptr->dirEntries[i].counter = count;
		memcpy(&objBlob->encArea + 1, dirL2Ptr, sizeof(cv_dir_page_level2)); 
		cryptLen = objBlob->objPageDirLen + sizeof(cv_obj_dir_page_blob_enc) + hmacLen;
		/* see if padding required */
		padLen = GET_AES_128_PAD_LEN(cryptLen);
		memset((uint8_t *)&objBlob->encArea + cryptLen, 0, padLen);
		cryptLen += padLen;
		objBlob->encAreaLen = cryptLen;
		/* compute auth length and pointer to HMAC */
		authLen = sizeof(cv_obj_dir_page_blob) + sizeof(cv_dir_page_level2);
		pHMAC = buf + authLen;
		/* now do encryption and auth */
		if ((retVal = cvEncryptBlob(buf, authLen, (uint8_t *)&objBlob->encArea, cryptLen,
			NULL, 0,
			objBlob->IV, CV_PERSISTENT_DATA->UCKKey, NULL, NULL, (uint8_t *)&hmacKey, pHMAC, hmacLen, CV_CTR,
			NULL, 0)) != CV_SUCCESS)
				goto err_exit;

		/* set up length for L2 entry */
		objDirEntry->length = cryptLen + sizeof(cv_obj_dir_page_blob) - sizeof(cv_obj_dir_page_blob_enc);
		/* level 1 */
		buf += objDirEntry->length; 
		objDirEntry = (cv_host_storage_object_entry *)buf;
		objDirEntry->handle = handles[0];
		buf = (uint8_t *)(objDirEntry + 1);
		objBlob = (cv_obj_dir_page_blob *)buf;
		objBlob->version.versionMajor = version.versionMajor;
		objBlob->version.versionMinor = version.versionMinor;
		objBlob->encArea.hObj = handles[0];
		objBlob->encArea.counter = count;
		objBlob->objPageDirLen = sizeof(cv_dir_page_level1);
		dirL1Ptr->header.thisDirPage.counter = count;
		if (!cvFindL2DirEntryInL1Dir(handles[1], dirL1Ptr, &i))
		{
			retVal = CV_INVALID_HANDLE;
			goto err_exit;
		}
		dirL1Ptr->dirEntries[i].counter = count;
		memcpy(&objBlob->encArea + 1, dirL1Ptr, sizeof(cv_dir_page_level1)); 
		cryptLen = objBlob->objPageDirLen + sizeof(cv_obj_dir_page_blob_enc) + hmacLen;
		/* see if padding required */
		padLen = GET_AES_128_PAD_LEN(cryptLen);
		memset((uint8_t *)&objBlob->encArea + cryptLen, 0, padLen);
		cryptLen += padLen;
		objBlob->encAreaLen = cryptLen;
		/* compute auth length and pointer to HMAC */
		authLen = sizeof(cv_obj_dir_page_blob) + sizeof(cv_dir_page_level1);
		pHMAC = buf + authLen;
		/* now do encryption and auth */
		if ((retVal = cvEncryptBlob(buf, authLen, (uint8_t *)&objBlob->encArea, cryptLen,
			NULL, 0,
			objBlob->IV, CV_PERSISTENT_DATA->UCKKey, NULL, NULL, (uint8_t *)&hmacKey, pHMAC, hmacLen, CV_CTR,
			NULL, 0)) != CV_SUCCESS)
				goto err_exit;
		/* set up length for L1 entry */
		objDirEntry->length = cryptLen + sizeof(cv_obj_dir_page_blob) - sizeof(cv_obj_dir_page_blob_enc);
		buf += objDirEntry->length;
		/* bump count for L1 entry in top level directory */
		if (!cvFindL1DirEntryInTopLevelDir(handles[0], &i))
		{
			retVal = CV_INVALID_HANDLE;
			goto err_exit;
		}
		CV_TOP_LEVEL_DIR->dirEntries[i].counter = count;
	} else if (objAttribs.storageType == CV_STORAGE_TYPE_FLASH 
		|| objAttribs.storageType == CV_STORAGE_TYPE_SMART_CARD
		|| objAttribs.storageType == CV_STORAGE_TYPE_CONTACTLESS
		|| objAttribs.storageType == CV_STORAGE_TYPE_FLASH_NON_OBJ) {
		/* not host hard drive, must be object in directory 0 or directory 0 itself.  bump count for directory 0 */
		/* and dir 0 entry in top level directory */
		/* if object, need to bump entry in dir 0 too */
		if (objAttribs.storageType != CV_STORAGE_TYPE_FLASH_NON_OBJ)
			((cv_dir_page_entry_obj_flash *)ptrs[1])->counter = count;
		CV_DIR_PAGE_0->header.thisDirPage.counter = count;
		CV_TOP_LEVEL_DIR->dirEntries[0].counter = count;
	}

	/* now update top level dir count */
	CV_TOP_LEVEL_DIR->header.thisDirPage.counter = count;

	
	if (objAttribs.storageType == CV_STORAGE_TYPE_HARD_DRIVE || objAttribs.storageType == CV_STORAGE_TYPE_HARD_DRIVE_NON_OBJ
				|| objAttribs.storageType == CV_STORAGE_TYPE_HARD_DRIVE_UCK)
	{
		/* set up total transport length */
		objStore->transportLen = buf - bufStart;
		*objLen = objStore->transportLen;
	}

err_exit:
	return retVal;
}

cv_status
cvUnwrapObj(cv_obj_properties *objProperties, cv_obj_storage_attributes objAttribs, cv_obj_handle *handles, uint8_t **ptrs, uint8_t **outPtrs)
{
	/* this routine decrypts, authenticates and does anti-replay check for object/directory page blobs */
	/* the handles list contains the object handle and if necessary, the handles for the directories */
	/* above the object in the hierarchy.  The object blobs are pointed to by the corresponding entry */
	/* in the 'ptrs' list and the 'outptrs' list contains the cache location for storing the resultant */
	/* object or directory page */
	uint32_t hmacLen, cryptLen, authLen;
	cv_session *session = NULL;
	cv_obj_dir_page_blob *objBlob, *dirBlob, *blob;
	cv_hmac_key hmacKey;
	uint8_t *AESKey2 = NULL, *AESKey1;
	uint8_t *pHMAC;
	cv_status retVal = CV_SUCCESS;
	uint32_t i;
	cv_dir_page_hdr *dirs[2];
	cv_dir_page_level1 *dirL1Ptr;
	cv_dir_page_level2 *dirL2Ptr;
	uint32_t count;
	cv_bool foundPrivacyKey = FALSE;
	uint32_t MRUIndex;
	cv_bool found = FALSE;
	uint8_t hmacKeyValue[14];

	if (objProperties != NULL)
		session = objProperties->session;
	/* set up directory pointers assuming in cache (ie, w/o blob wrapper) */
	dirs[0] = (cv_dir_page_hdr *)ptrs[0];
	dirs[1] = (cv_dir_page_hdr *)ptrs[1];

	/* first decrypt object, if one */
	objBlob = (cv_obj_dir_page_blob *)ptrs[2];
	hmacLen = SHA256_LEN;
	/* compute HMAC key */
	memcpy(hmacKeyValue, "CV object blob", sizeof(hmacKeyValue));
	if ((cvAuth(hmacKeyValue, sizeof(hmacKeyValue), NULL, (uint8_t *)&hmacKey, hmacLen, NULL, 0, CV_SHA)) != CV_SUCCESS)
	{
		retVal = CV_OBJ_AUTH_FAIL;
		goto err_exit;
	}
	
	/* Since DIR_PAGE0_HANDLE is defined as 0x0, check storage type as well */
	if (handles[2] || (objAttribs.storageType == CV_STORAGE_TYPE_FLASH_NON_OBJ))
	{
		cryptLen = objBlob->encAreaLen;
		authLen = sizeof(cv_obj_dir_page_blob) + objBlob->objPageDirLen;
		//authLen = cryptLen + sizeof(cv_obj_dir_page_blob) - sizeof(cv_obj_dir_page_blob_enc);
		/* get pointer to decrypted HMAC */
		pHMAC = (uint8_t *)(objBlob + 1) + objBlob->objPageDirLen;
		/* check to see if privacy key being used by this session */
		if (session != NULL)
		{
			if (session->flags & CV_HAS_PRIVACY_KEY)
				AESKey2 = session->privacyKey;
			else {
				/* need to handle the case where session doesn't use privacy key, but object originally encrypted */
				/* with one (eg for backup).  in this case, leave object encrypted with privacy key */
				/* if IV non-zero, privacy key was used */
				AESKey2 = NULL;
				if (objProperties != NULL)
				{
					for (i=0;i<AES_128_IV_LEN;i++)
					{
						if (objBlob->IVPrivKey[i] !=0)
						{
							foundPrivacyKey = TRUE;
							/* object has privacy key encryption.  if upper level routines didn't indicate this is ok */
							/* it's an error.  if ok, just indicate in objProperties for upper level routines */
							if (objProperties->attribs.hasPrivacyKey)
							{
								objProperties->attribs = objAttribs;
								objProperties->attribs.hasPrivacyKey = 1;
								memcpy(objProperties->pIVPriv, objBlob->IVPrivKey, AES_128_IV_LEN);
							} else {
								/* this is an error.  object has privacy key encryption, but session doesn't */
								retVal = CV_OBJECT_REQUIRES_PRIVACY_KEY;
								goto err_exit;
							}
							break;
						}
					}
					if (!foundPrivacyKey)
						/* indicate to upper layer routines that now privacy key found */
						objProperties->attribs.hasPrivacyKey = 0;

					/* save the version of the obj as an obj property */
					objProperties->version = objBlob->version;
				}
			}
		}
		/* check for special cases for flash keys */
		if (objAttribs.storageType == CV_STORAGE_TYPE_FLASH_NON_OBJ || objAttribs.storageType == CV_STORAGE_TYPE_HARD_DRIVE_NON_OBJ)
			/* if not an object, don't use privacy key */
			AESKey2 = NULL;
		/* if exporting object which already has privacy encryption, don't use privacy key, and save IV for export */
		if (objAttribs.hasPrivacyKey)
		{
			AESKey2 = NULL;
			if (objProperties != NULL)
				memcpy(objProperties->pIVPriv, objBlob->IVPrivKey, AES_128_IV_LEN);
		}
		if (objAttribs.storageType == CV_STORAGE_TYPE_FLASH_PERSIST_DATA)
		{
			/* use 6T flash key to encrypt persistent data */
			AESKey1 = NV_6TOTP_DATA->Kaes;
			AESKey2 = NULL;
		} else if (objAttribs.storageType == CV_STORAGE_TYPE_HARD_DRIVE_UCK)
			/* UCK is encrypted with GCK */
			AESKey1 = CV_PERSISTENT_DATA->GCKKey;
		else
			AESKey1 = CV_PERSISTENT_DATA->UCKKey;
		/* when doing decryption with privacy key, only decrypt object (excluding object header) */
		/* validate length to avoid crash in bulk decrypt DMA */
		if (cryptLen > FP_CAPTURE_LARGE_HEAP_SIZE)
		{
                        retVal = CV_OBJ_DECRYPTION_FAIL;
                        goto err_exit;
                }
		if ((retVal = cvDecryptBlob((uint8_t *)objBlob, authLen, (uint8_t *)&objBlob->encArea, cryptLen,
			(uint8_t *)(&objBlob->encArea + 1) + sizeof(cv_obj_hdr), objBlob->objPageDirLen - sizeof(cv_obj_hdr),
			objBlob->IV, AESKey1, objBlob->IVPrivKey, AESKey2, (uint8_t *)&hmacKey, 
			pHMAC, hmacLen, CV_CBC, NULL, 0)) != CV_SUCCESS)
		{
			retVal = CV_OBJ_DECRYPTION_FAIL;
			goto err_exit;
		}
		/* validate the object */
		if (objBlob->encArea.hObj != handles[2])
		{
			retVal = CV_OBJ_VALIDATION_FAIL;
			goto err_exit;
		}
	
	}

	/* now decrypt directory pages for host hard drive objects (non-mirrored), if necessary */
	if (objAttribs.storageType == CV_STORAGE_TYPE_HARD_DRIVE && (handles[2] & CV_OBJ_HANDLE_PAGE_MASK) != CV_MIRRORED_OBJ_MASK)
	{
		for (i=0;i<(MAX_DIR_LEVELS-1);i++)
		{
			if (handles[i])
			{
				dirBlob = (cv_obj_dir_page_blob *)ptrs[i];
				dirs[i] = (cv_dir_page_hdr *)&dirBlob->encArea.hObj;
				cryptLen = dirBlob->encAreaLen;
				authLen = sizeof(cv_obj_dir_page_blob) + dirBlob->objPageDirLen;
				//authLen = cryptLen + sizeof(cv_obj_dir_page_blob) - sizeof(cv_obj_dir_page_blob_enc);
				/* get pointer to decrypted HMAC */
				pHMAC = (uint8_t *)(dirBlob + 1) + dirBlob->objPageDirLen;
				if ((retVal = cvDecryptBlob((uint8_t *)dirBlob, authLen, (uint8_t *)&dirBlob->encArea, cryptLen,
					NULL, 0,
					dirBlob->IV, CV_PERSISTENT_DATA->UCKKey, NULL, NULL, (uint8_t *)&hmacKey, 
					pHMAC, hmacLen, CV_CTR, NULL, 0)) != CV_SUCCESS)
				{
					retVal = CV_OBJ_DECRYPTION_FAIL;
					goto err_exit;
				}
				/* validate the object */
				if (dirBlob->encArea.hObj != handles[i])
				{
					retVal = CV_OBJ_VALIDATION_FAIL;
					goto err_exit;
				}
			}
		}

		/* now check anti-replay counts.  first check object count vs directory page containing it */
		/* handle directory page 0 differently, since directory structure different and directory is resident */
		if (CV_VOLATILE_DATA->CVDeviceState & CV_2T_ACTIVE)
		{
			if (objAttribs.storageType == CV_STORAGE_TYPE_HARD_DRIVE && (handles[2] & CV_OBJ_HANDLE_PAGE_MASK) != CV_MIRRORED_OBJ_MASK)
			{
				/* must check all counts up to top level directory */
				/* set up pointers to directories.  note that if the directory was retrieved, it will point to the blob header */
				/* and if not (ie, in cache) it will point directly to the directory */
				if (handles[0])
					dirL1Ptr = (cv_dir_page_level1 *)((uint8_t *)dirs[0] + sizeof(cv_obj_dir_page_blob_enc));
				else
					dirL1Ptr = (cv_dir_page_level1 *)dirs[0];
				if (handles[1])
					dirL2Ptr = (cv_dir_page_level2 *)((uint8_t *)dirs[1] + sizeof(cv_obj_dir_page_blob_enc));
				else
					dirL2Ptr = (cv_dir_page_level2 *)dirs[1];
				if (handles[2])
				{
					/* if object retrieved, L2 dir must have been retrieved or be in cache */
					if (!dirL2Ptr)
					{
						if (!cvFindDirPageInL2Cache(handles[2], &dirL2Ptr, &MRUIndex))
						{
							/* fail, because can't do anti-replay check */
							retVal = CV_OBJ_ANTIREPLAY_FAIL;
							goto err_exit;
						}
					}
					/* find object in level 2 directory */
					found = cvFindObjEntryInL2Dir(handles[2], dirL2Ptr, &i);
					if (!found || objBlob->encArea.counter != dirL2Ptr->dirEntries[i].counter)
					{
						retVal = CV_OBJ_ANTIREPLAY_FAIL;
						goto err_exit;
					}
				}

				if (handles[1] || handles[2])
				{
					/* if L2 dir or object retrieved, L1 dir must have been retrieved or be in cache */
					if (!dirL1Ptr)
					{
						if (!cvFindDirPageInL1Cache(handles[1], &dirL1Ptr, &MRUIndex))
						{
							/* fail, because can't do anti-replay check */
							retVal = CV_OBJ_ANTIREPLAY_FAIL;
							goto err_exit;
						}
					}
					/* find level 2 directory in level 1 directory */
					found = cvFindL2DirEntryInL1Dir(dirL2Ptr->header.thisDirPage.hDirPage, dirL1Ptr, &i);
					if (!found || dirL2Ptr->header.thisDirPage.counter != dirL1Ptr->dirEntries[i].counter)
					{
						retVal = CV_OBJ_ANTIREPLAY_FAIL;
						goto err_exit;
					}
				}

				if (handles[0] || handles[1] || handles[2])
				{
					/* find level 1 directory in top level directory */
					found = cvFindL1DirEntryInTopLevelDir(dirL1Ptr->header.thisDirPage.hDirPage, &i);
					if (!found || dirL1Ptr->header.thisDirPage.counter != CV_TOP_LEVEL_DIR->dirEntries[i].counter)
					{
						retVal = CV_OBJ_ANTIREPLAY_FAIL;
						goto err_exit;
					}
				}
			}
		}
	} 
	else if (CV_VOLATILE_DATA->CVDeviceState & CV_2T_ACTIVE)
	{

		if (objAttribs.storageType == CV_STORAGE_TYPE_FLASH 
			|| objAttribs.storageType == CV_STORAGE_TYPE_SMART_CARD
			|| objAttribs.storageType == CV_STORAGE_TYPE_CONTACTLESS) {
			/* not host hard drive, must be object in directory 0.  check count for directory 0 */
			if (objBlob->encArea.counter != ((cv_dir_page_entry_obj_flash *)ptrs[1])->counter)
			{
				retVal = CV_OBJ_ANTIREPLAY_FAIL;
				goto err_exit;
			}
		} 
		
		else {
			/* if here, must be either directory 0, top level directory or persistent data */
			if (objBlob->encArea.hObj == CV_DIR_0_HANDLE)
			{
				/* directory 0, compare with entry in top level dir */
				CV_VOLATILE_DATA->actualCount = objBlob->encArea.counter;
				if (objBlob->encArea.counter != CV_TOP_LEVEL_DIR->dirEntries[0].counter)
				{
					retVal = CV_OBJ_ANTIREPLAY_FAIL;
					goto err_exit;
				}
			} else if (objBlob->encArea.hObj == CV_TOP_LEVEL_DIR_HANDLE) {
				/* top level directory, compare with counter */
				if ((retVal = get_monotonic_counter(&count)) != CV_SUCCESS)
					goto err_exit;
				CV_VOLATILE_DATA->actualCount = objBlob->encArea.counter;
				/* allow override of anti-replay in order to handle recovery scenario (when flash write failure occurs) */
				if (objBlob->encArea.counter != count && !objAttribs.replayOverride)
				{
					retVal = CV_OBJ_ANTIREPLAY_FAIL;
					goto err_exit;
				}
			} else if (objBlob->encArea.hObj == CV_PERSISTENT_DATA_HANDLE) {
				/* this is the persistent data object.  compare count against entry in top level dir */
				CV_VOLATILE_DATA->actualCount = objBlob->encArea.counter;
				if (objBlob->encArea.counter != CV_TOP_LEVEL_DIR->persistentData.entry.counter)
				{
					retVal = CV_OBJ_ANTIREPLAY_FAIL;
					goto err_exit;
				}
			} else if (objBlob->encArea.hObj == CV_UCK_OBJ_HANDLE) {
				/* this is the UCK.  instead of comparing count, compare value, which is stored in persistent data */
				blob = (cv_obj_dir_page_blob *)ptrs[2];
				if (memcmp((uint8_t *)(blob + 1), CV_PERSISTENT_DATA->UCKKey, AES_128_BLOCK_LEN))
				{
					retVal = CV_OBJ_ANTIREPLAY_FAIL;
					goto err_exit;
				}
			}
			/* if none of the above, must be mirrored object, so don't check anti-replay count.  mirrored objects */
			/* are only read in when being restored and CV admin has authorized anti-replay override */
		}
	}

	/* now copy validated object and any directory pages that were retrieved to destination */
	for (i=0;i<MAX_DIR_LEVELS;i++)
	{
		blob = (cv_obj_dir_page_blob *)ptrs[i];
		if (handles[i])
		{
			uint32_t length = blob->objPageDirLen;

			/* this is to workaround a bug where the objPageDirLen for the persistent data was set incorrectly (too long) in blob */
			if (outPtrs[i] == (uint8_t *)CV_PERSISTENT_DATA)
				length = sizeof(cv_persistent);
			memcpy(outPtrs[i], (uint8_t *)(blob + 1), length);
		}
	}
	
	/* Special case for DIR0 with handle 0x0 */
	if ((handles[2] == 0) && (objAttribs.storageType == CV_STORAGE_TYPE_FLASH_NON_OBJ)) 
	{
		blob = (cv_obj_dir_page_blob *)ptrs[2];		
		memcpy(outPtrs[2], (uint8_t *)(blob + 1), blob->objPageDirLen);
	}
err_exit:
	return retVal;
}

cv_status
cvGetFlashObj(cv_dir_page_entry_obj_flash *objEntry, uint8_t *objPtr)
{
	return ush_read_flash((uint32_t)objEntry->pObj, objEntry->objLen, objPtr);
}

cv_status
cvGetHostObj(cv_obj_handle *handles, uint8_t **ptrs, uint8_t **outPtrs)
{
	/* this routine gets an object (and associated directory pages, if indicated) from the host hard drive */
	cv_host_storage_retrieve_object_in *objReq = (cv_host_storage_retrieve_object_in *)CV_SECURE_IO_COMMAND_BUF;
	cv_host_storage_retrieve_object_out *objRes = (cv_host_storage_retrieve_object_out *)CV_SECURE_IO_COMMAND_BUF;
	uint32_t startTime;
	cv_status retVal = CV_SUCCESS;
	cv_usb_interrupt *usbInt;
	uint32_t i, j=0;
	cv_obj_handle *listPtr;
	uint32_t align;		/* 4-byte alignment pad required for usb InterruptIn for cases where BulkIn data is not multiple of 4bytes */
	typedef PACKED struct td_result_list
	{
		cv_obj_handle	handle;
		uint32_t		len;
	} result_list;
	 result_list *resPtr;
	cv_bool found;
	uint8_t *endPtr;
	
	/* set up host storage request */
	objReq->transportType = CV_TRANS_TYPE_HOST_STORAGE;
	objReq->commandID = CV_HOST_STORAGE_RETRIEVE_OBJECT;
	/* now add obj and dir page handles */
	listPtr = (cv_obj_handle *)((uint8_t *)(objReq + 1));
	for (i=0;i<MAX_DIR_LEVELS;i++)
		if (handles[i])
			listPtr[j++] = handles[i];
	objReq->transportLen = sizeof(cv_host_storage_retrieve_object_in) + j*sizeof(cv_obj_handle);

	align = ALIGN_DWORD(objReq->transportLen);
	
	/* first post buffer to host to receive data from host.  it's ok to use the same buffer that is posted */
	/* as output to host, because no data from host will be received until host storage request has been processed */
	/* set up interrupt after command */
	usbInt = (cv_usb_interrupt *)((uint8_t *)objReq + align);
#ifdef HD_DEBUG	
	usbInt->interruptType = CV_COMMAND_PROCESSING_INTERRUPT;
	objReq->transportType = CV_TRANS_TYPE_ENCAPSULATED;
#else
	usbInt->interruptType = CV_HOST_STORAGE_INTERRUPT;
#endif	
	usbInt->resultLen = objReq->transportLen;

	/* before switching to open mode, zero all memory in window after command */
	memset(usbInt + 1, 0, CV_IO_COMMAND_BUF_SIZE - (objReq->transportLen + sizeof(cv_usb_interrupt))); 
	set_smem_openwin_open(CV_IO_OPEN_WINDOW);
	
	/* now wait for USB transfer complete */
	if (!usbCvSend(usbInt, (uint8_t *)objReq, objReq->transportLen, HOST_STORAGE_REQUEST_TIMEOUT)) {
		retVal = CV_HOST_STORAGE_REQUEST_TIMEOUT;
		goto err_exit;
	}

	/* Wait for status command from Host */
	//usbCvPrepareRxEpt();
	usbCvBulkOut((cv_encap_command *)objReq);
	
	get_ms_ticks(&startTime);
	do
	{
		if(cvGetDeltaTime(startTime) >= HOST_STORAGE_REQUEST_TIMEOUT)
		{
			retVal = CV_HOST_STORAGE_REQUEST_TIMEOUT;
			goto err_exit;
		}

		yield_and_delay(100);
		
		if (usbCvPollCmdRcvd())
			break;
	} while (TRUE);
	
	/* now set up the object/dir page pointers for unwrapping */
	if (objRes->transportLen <= sizeof(cv_host_storage_retrieve_object_out))
	{
		retVal = CV_HOST_STORAGE_REQUEST_RESULT_INVALID;
		goto err_exit;
	}
	/* check to see if request was successful */
	if (objRes->status != CV_SUCCESS)
	{
		retVal = objRes->status;
		goto err_exit;
	}
	resPtr = (result_list *)((uint8_t *)(objRes + 1));
	endPtr = (uint8_t *)objReq + objReq->transportLen;
	/* parse result list */
	do
	{
		/* find handle corresponding to object/dir page received */
		found = FALSE;
		for (j=0;j<MAX_DIR_LEVELS;j++) {
			if (resPtr->handle == handles[j]) {
				found = TRUE;
				break;
			}
		}
		if (!found)
		{
			retVal = CV_HOST_STORAGE_REQUEST_RESULT_INVALID;
			goto err_exit;
		}
		/* output pointer is cache location; pointer is in io buf.  unwrap will copy from io buf to cache */
		outPtrs[j] = ptrs[j];
		ptrs[j] = (uint8_t *)(resPtr + 1);
		resPtr = (result_list *)((uint8_t *)(resPtr + 1) + resPtr->len);
	} while ((uint8_t *)resPtr < endPtr);

	/* now make sure all requested handles were retrieved */
	for (i=0;i<MAX_DIR_LEVELS;i++)
	{
		if (handles[i] && !outPtrs[i])
		{
			retVal = CV_HOST_STORAGE_REQUEST_RESULT_INVALID;
			goto err_exit;
		}
	}

err_exit:
	set_smem_openwin_secure(CV_IO_OPEN_WINDOW);
	return retVal;
}

cv_status
cvGetObj(cv_obj_properties *objProperties, cv_obj_hdr **objPtr)
{
	/* this function retrieves the indicated object into scratch RAM */
	cv_status retVal = CV_SUCCESS, retValSaved;
	cv_obj_handle objHandle = objProperties->objHandle;
	uint32_t i,j;
	char objID[] = "cvobj_xxxxxxxx";
	uint32_t retLen;
	uint32_t objCacheEntry = INVALID_CV_CACHE_ENTRY, L1CacheEntry = INVALID_CV_CACHE_ENTRY, L2CacheEntry = INVALID_CV_CACHE_ENTRY;
	uint8_t *objStoragePtr;
	cv_obj_handle objDirHandles[3];
	uint8_t *objDirPtrs[3], *objDirPtrsOut[3];
	cv_obj_storage_attributes objAttribs;
	uint32_t count, predictedCount;

	/* fail if obj handle null */
	if (objHandle == 0)
	{
		retVal = CV_INVALID_OBJECT_HANDLE;
		goto err_exit;
	}

	/* first search for the object in the object cache */
	if (cvFindObjInCache(objHandle, objPtr, &objCacheEntry))
	{
		/* set this cache entry to MRU */
		cvUpdateObjCacheMRU(objCacheEntry);
		goto obj_found;
	}
	/* not in object cache, need to read in (unless in volatile obj directory) */
	objDirHandles[0] = objDirHandles[1] = 0;
	objDirHandles[2] = objHandle;
	objDirPtrs[0] = objDirPtrs[1] = NULL;
	objDirPtrsOut[0] = objDirPtrsOut[1] = NULL;
	/* check to see if object is in directory page 0 */
	if (GET_DIR_PAGE(objHandle) == 0)
	{
		/* yes, first search volatile object directory */
		if (cvFindVolatileDirEntry(objHandle, &i))
		{
			*objPtr = (cv_obj_hdr *)CV_VOLATILE_OBJ_DIR->volObjs[i].pObj;
			goto obj_found;
		}
		/* now search directory page 0 */
		if (cvFindDir0Entry(objHandle, &i))
		{
			/* object found, determine where object resident.  then read in */
			/* get slot in object cache */
			cvObjCacheGetLRU(&objCacheEntry, &objStoragePtr);
			objDirPtrs[2] = objStoragePtr;
			objDirPtrsOut[2] = objStoragePtr;
			objDirPtrs[1] = (uint8_t *)&CV_DIR_PAGE_0->dir0Objs[i];
			objAttribs = CV_DIR_PAGE_0->dir0Objs[i].attributes;
			switch (objAttribs.storageType)
			{
			case CV_STORAGE_TYPE_FLASH:
				if ((retVal = cvGetFlashObj(&CV_DIR_PAGE_0->dir0Objs[i], objStoragePtr)) != CV_SUCCESS)
				{
					cv_hdotp_skip_table *pHdotpSkipTable = (cv_hdotp_skip_table *)((CHIP_IS_5882) ? CV_SKIP_TABLE_BUFFER : FP_CAPTURE_LARGE_HEAP_PTR);

					/* if not able to read object, remove from directory */
					retValSaved = retVal;
					CV_DIR_PAGE_0->dir0Objs[i].hObj = 0;
					/* now check to see if anti-hammering will allow 2T usage */
					if ((retVal = cvCheckAntiHammeringStatus()) != CV_SUCCESS)
						goto err_exit;
					/* now bump counter, so that dir page 0 will get new value */
					/* loop with 2T bump last, so that failure during process will cause retry of entire process */

					/* read skip tables */
					cvReadHDOTPSkipTable(pHdotpSkipTable);
					for (j=0;j<CV_MAX_2T_BUMP_FAIL;j++)
					{
						/* first do predictive bump to get next 2T value */
						if (get_predicted_monotonic_counter(&predictedCount) != CV_SUCCESS)
							goto err_exit;
						/* write out updated dir page 0 */
						if ((retVal = cvWriteDirectoryPage0(TRUE)) != CV_SUCCESS)
							continue;
						/* also write out TLD */
						if ((retVal = cvWriteTopLevelDir(TRUE)) != CV_SUCCESS)
							continue;
						/* write out HDOTP skip table */
						if ((retVal = cvWriteHDOTPSkipTable()) != CV_SUCCESS)
							continue;
						/* now try to bump 2T */
						if ((retVal = bump_monotonic_counter(&count)) != CV_SUCCESS)
							continue;
						/* if 2T succeeded, make sure matches predicted count */
						if (count == predictedCount)
						{
							/* successful, zero retry flags */
							CV_VOLATILE_DATA->retryWrites = 0;
							break;
						}
					}
					/* check to see if loop terminated successfully */
					if (retVal == CV_SUCCESS)
					{
						if (j == CV_MAX_2T_BUMP_FAIL)
							/* count never matched predicted value */
							retVal = CV_MONOTONIC_COUNTER_FAIL;
						else
							/* otherwise, use initial write failure */
							retVal = retValSaved;
					}
					goto err_exit;
				}
				break;
			case CV_STORAGE_TYPE_SMART_CARD:
				sprintf(objID + 6,"%08x",objHandle);
				retLen = CV_OBJ_CACHE_ENTRY_SIZE;
				if ((retVal = scGetObj(sizeof(objID), objID, objProperties->PINLen, objProperties->PIN, 
					&retLen, objStoragePtr)) != CV_SUCCESS)
						goto err_exit;
				break;
			case CV_STORAGE_TYPE_CONTACTLESS:
				sprintf(objID + 6,"%08x",objHandle);
				retLen = CV_OBJ_CACHE_ENTRY_SIZE;
				if ((retVal = rfGetObj(sizeof(objID), objID, objProperties->PINLen, objProperties->PIN, 
					&retLen, objStoragePtr)) != CV_SUCCESS)
						goto err_exit;
				break;
			case 0xffff:
				break;
			}
		} else {
			/* object not found, but handle indicated it should have been in directory page 0, so fail */
			retVal = CV_INVALID_OBJECT_HANDLE;
			goto err_exit;
		}
	} else {
		/* not directory page 0, which means object stored on host hard drive */
		/* get slot in object cache */
		/* disable host storage objects */
		retVal = CV_INVALID_OBJECT_HANDLE;
		goto err_exit;
#if 0
		objAttribs.storageType = CV_STORAGE_TYPE_HARD_DRIVE;
		cvObjCacheGetLRU(&objCacheEntry, &objStoragePtr);
		objDirPtrs[2] = objStoragePtr;
		/* first search directory page caches for directory pages in hierarchy above this object */
		if (!cvFindDirPageInL1Cache(objHandle, (cv_dir_page_level1 **)&objDirPtrs[0], &L1CacheEntry))
		{
			/* not in dir page cache, need to read in.  get slot in dir page cache */
			cvLevel1DirCacheGetLRU(&L1CacheEntry, &objDirPtrs[0]);
			objDirHandles[0] = GET_L1_HANDLE_FROM_OBJ_HANDLE(objHandle);
		}
		if (!cvFindDirPageInL2Cache(objHandle, (cv_dir_page_level2 **)&objDirPtrs[1], &L2CacheEntry))
		{
			/* not in dir page cache, need to read in.  get slot in dir page cache */
			cvLevel2DirCacheGetLRU(&L2CacheEntry, &objDirPtrs[1]);
			objDirHandles[1] = GET_L2_HANDLE_FROM_OBJ_HANDLE(objHandle);
		}
		/* now retrieve object and any directory pages not in cache */
		if ((retVal = cvGetHostObj(&objDirHandles[0], &objDirPtrs[0], &objDirPtrsOut[0])) != CV_SUCCESS)
			goto err_exit;
#endif
	}
	/* now unwrap object and any directory pages read in */
	if ((retVal = cvUnwrapObj(objProperties, objAttribs, &objDirHandles[0], &objDirPtrs[0], &objDirPtrsOut[0])) != CV_SUCCESS)
		goto err_exit;
	*objPtr = (cv_obj_hdr *)objStoragePtr;
	/* put this object handle into cache list and update MRU list */
	CV_VOLATILE_DATA->objCache[objCacheEntry] = objHandle;
	cvUpdateObjCacheMRU(objCacheEntry);
#if 0
	/* also update cache lists for directory pages, if any were retrieved */
	if (L1CacheEntry != INVALID_CV_CACHE_ENTRY)
	{
		CV_VOLATILE_DATA->level1DirCache[L1CacheEntry] = objDirHandles[0];
		cvUpdateLevel1CacheMRU(L1CacheEntry);
	}
	if (L2CacheEntry != INVALID_CV_CACHE_ENTRY)
	{
		CV_VOLATILE_DATA->level1DirCache[L2CacheEntry] = objDirHandles[1];
		cvUpdateLevel2CacheMRU(L2CacheEntry);
	}
#endif

err_exit:
obj_found:
	/* if object found, parse to fill in fields in objProperties */
	if (retVal == CV_SUCCESS)
		/* if object is privacy encrypted, can't get pointers (this is for export for backup case */
		/* volatile objects never have privacy encryption, so ignore this flag setting */
		if (!objProperties->attribs.hasPrivacyKey || objProperties->attribs.storageType == CV_STORAGE_TYPE_VOLATILE)
			cvFindObjPtrs(objProperties, *objPtr);
	/* invalidate HDOTP skip table here so that buffer (FP capture buffer) can be reused */
	cvInvalidateHDOTPSkipTable();
	return retVal;
}

cv_status
cvDelHostObj(cv_obj_handle handle, cv_host_storage_delete_object *objOut)
{
	/* this routine deletes an object from the host hard drive */
	uint32_t startTime;
	cv_status retVal = CV_SUCCESS;
	cv_host_storage_operation_status *objRes = (cv_host_storage_operation_status *)CV_SECURE_IO_COMMAND_BUF;
	cv_usb_interrupt *usbInt;
	uint32_t align;		/* 4-byte alignment pad required for usb InterruptIn for cases where BulkIn data is not multiple of 4bytes */

	align = ALIGN_DWORD(objOut->transportLen);
	
	/* first set up delete request */
	objOut->transportType = CV_TRANS_TYPE_HOST_STORAGE;
	objOut->transportLen = sizeof(cv_host_storage_delete_object);
	objOut->commandID = CV_HOST_STORAGE_DELETE_OBJECT;
	objOut->objectID = handle;

	/* now do output */
	usbInt = (cv_usb_interrupt *)((uint8_t *)objOut + align);

#ifdef HD_DEBUG	
	usbInt->interruptType = CV_COMMAND_PROCESSING_INTERRUPT;
	objOut->transportType = CV_TRANS_TYPE_ENCAPSULATED;
#else
	usbInt->interruptType = CV_HOST_STORAGE_INTERRUPT;
#endif	
	usbInt->resultLen = objOut->transportLen;

	/* before switching to open mode, zero all memory in window after command */
	memset(usbInt + 1, 0, CV_IO_COMMAND_BUF_SIZE - (objOut->transportLen + sizeof(cv_usb_interrupt))); 
	set_smem_openwin_open(CV_IO_OPEN_WINDOW);
	
	/* now wait for USB transfer complete */
	if (!usbCvSend(usbInt, (uint8_t *)objOut, objOut->transportLen, HOST_STORAGE_REQUEST_TIMEOUT)) {
		retVal = CV_HOST_STORAGE_REQUEST_TIMEOUT;
		goto err_exit;
	}
	
	/* Wait for status command from Host */
	usbCvBulkOut((cv_encap_command *)objRes);
	//usbCvPrepareRxEpt();
			
	get_ms_ticks(&startTime);
	do
	{
		/* Longer timeout required for receiving host command.. need to tweak later */
		if(cvGetDeltaTime(startTime) >= HOST_STORAGE_REQUEST_TIMEOUT)

		{
			retVal = CV_HOST_STORAGE_REQUEST_TIMEOUT;
			goto err_exit;
		}

		yield_and_delay(100);
		
		if (usbCvPollCmdRcvd())
			break;
	} while (TRUE);

err_exit:
	set_smem_openwin_secure(CV_IO_OPEN_WINDOW);
	return retVal;
}

cv_status
cvDelAllHostObjs(void)
{
	/* this routine deletes all objects from the host hard drive */
	uint32_t startTime;
	cv_status retVal = CV_SUCCESS;
	cv_host_storage_delete_all_files *objOut = (cv_host_storage_delete_all_files *)CV_SECURE_IO_COMMAND_BUF;
	cv_host_storage_operation_status *objRes = (cv_host_storage_operation_status *)CV_SECURE_IO_COMMAND_BUF;
	cv_usb_interrupt *usbInt;
	uint32_t align;		/* 4-byte alignment pad required for usb InterruptIn for cases where BulkIn data is not multiple of 4bytes */

	/* first set up delete request */
	objOut->transportType = CV_TRANS_TYPE_HOST_STORAGE;
	objOut->transportLen = sizeof(cv_transport_header);
	objOut->commandID = CV_HOST_STORAGE_DELETE_ALL_FILES;
	align = ALIGN_DWORD(objOut->transportLen);

	/* now do output */
	usbInt = (cv_usb_interrupt *)((uint8_t *)objOut + align);

	usbInt->interruptType = CV_HOST_STORAGE_INTERRUPT;
	usbInt->resultLen = objOut->transportLen;

	/* before switching to open mode, zero all memory in window after command */
	memset(usbInt + 1, 0, CV_IO_COMMAND_BUF_SIZE - (objOut->transportLen + sizeof(cv_usb_interrupt))); 
	set_smem_openwin_open(CV_IO_OPEN_WINDOW);
	
	/* now wait for USB transfer complete */
	if (!usbCvSend(usbInt, (uint8_t *)objOut, objOut->transportLen, HOST_STORAGE_REQUEST_TIMEOUT)) {
		retVal = CV_HOST_STORAGE_REQUEST_TIMEOUT;
		goto err_exit;
	}
	
	/* Wait for status command from Host */
	usbCvBulkOut((cv_encap_command *)objRes);
	//usbCvPrepareRxEpt();
			
	get_ms_ticks(&startTime);
	do
	{
		/* Longer timeout required for receiving host command.. need to tweak later */
		if(cvGetDeltaTime(startTime) >= HOST_STORAGE_REQUEST_TIMEOUT)

		{
			retVal = CV_HOST_STORAGE_REQUEST_TIMEOUT;
			goto err_exit;
		}

		yield_and_delay(100);
		
		if (usbCvPollCmdRcvd())
			break;
	} while (TRUE);

err_exit:
	set_smem_openwin_secure(CV_IO_OPEN_WINDOW);
	return retVal;
}

cv_status
cvPutFlashObj(cv_dir_page_entry_obj_flash *objEntry, uint8_t *objPtr)
{
	return ush_write_flash((uint32_t)objEntry->pObj, objEntry->objLen, objPtr);
}

cv_status
cvPutHostObj(cv_host_storage_store_object *objOut)
{
	/* this routine puts an object (and associated directory pages, if indicated) to the host hard drive */
	uint32_t startTime;
	cv_status retVal = CV_SUCCESS;
	cv_usb_interrupt *usbInt;
	cv_host_storage_operation_status *objRes = (cv_host_storage_operation_status *)CV_SECURE_IO_COMMAND_BUF;
	uint32_t align;		/* 4-byte alignment pad required for usb InterruptIn for cases where BulkIn data is not multiple of 4bytes */
	
	align = ALIGN_DWORD(objOut->transportLen);

	/* request already set up by cvWrapObj, just do output */
	/* first post buffer to host to receive data from host.  it's ok to use the same buffer that is posted */
	/* as output to host, because no data from host will be received until host storage request has been processed */
	/* set up interrupt after command */
	//usbInt = (cv_usb_interrupt *)((uint8_t *)objOut + objOut->transportLen + align);
	usbInt = (cv_usb_interrupt *)((uint8_t *)objOut + align);
#ifdef HD_DEBUG	
	usbInt->interruptType = CV_COMMAND_PROCESSING_INTERRUPT;
	objOut->transportType = CV_TRANS_TYPE_ENCAPSULATED;
#else	
	usbInt->interruptType = CV_HOST_STORAGE_INTERRUPT;
#endif	
	usbInt->resultLen = objOut->transportLen;

	/* before switching to open mode, zero all memory in window after command */
	//memset(CV_SECURE_IO_COMMAND_BUF + objOut->transportLen + align + sizeof(cv_usb_interrupt), 0, CV_IO_COMMAND_BUF_SIZE - (objOut->transportLen + sizeof(cv_usb_interrupt))); 
	memset(usbInt + 1, 0, CV_IO_COMMAND_BUF_SIZE - (objOut->transportLen + sizeof(cv_usb_interrupt))); 
	set_smem_openwin_open(CV_IO_OPEN_WINDOW);

	/* now wait for USB transfer complete */
	if (!usbCvSend(usbInt, (uint8_t *)objOut, objOut->transportLen, HOST_STORAGE_REQUEST_TIMEOUT)) {
		retVal = CV_HOST_STORAGE_REQUEST_TIMEOUT;
		goto err_exit;
	}

	/* Wait for status command from Host */
	//usbCvPrepareRxEpt();
	usbCvBulkOut((cv_encap_command *)objRes);
		
	get_ms_ticks(&startTime);
	do
	{
		/* Longer timeout required for receiving host command.. need to tweak later */
		if(cvGetDeltaTime(startTime) >= HOST_STORAGE_REQUEST_TIMEOUT)
		{
			retVal = CV_HOST_STORAGE_REQUEST_TIMEOUT;
			goto err_exit;
		}

		yield_and_delay(100);
		
		if (usbCvPollCmdRcvd())
			break;
	} while (TRUE);

	/* now check the status of the return */
	if (objRes->transportLen <= sizeof(cv_host_storage_operation_status))
	{
		retVal = CV_HOST_STORAGE_REQUEST_RESULT_INVALID;
		goto err_exit;
	}
	/* check to see if request was successful */
	//if (objRes->status != CV_SUCCESS)
	retVal = objRes->status;

err_exit:
	set_smem_openwin_secure(CV_IO_OPEN_WINDOW);
	return retVal;
}

cv_status
cvFindObjAtribFlags(cv_obj_properties *objProperties, cv_obj_hdr *objPtr, cv_attrib_flags **flags, cv_obj_storage_attributes *attributes)
{
	/* this routine searches through object attributes and gets a pointer to the attribute flags */
	cv_attrib_hdr *attribHdrPtr, *attribEndPtr;
	cv_status retVal = CV_SUCCESS;
	cv_bool found = FALSE;

	/* first, handle case where this is a privacy encrypted object.  if so, attributes in object are not directly accessible */
	/* so attributes which have been stored in objProperties are used instead */
	if (objProperties->attribs.hasPrivacyKey)
	{
		/* object is privacy encrypted */
		*attributes = objProperties->attribs;
		*flags = NULL;
		return CV_SUCCESS;
	}
	attribHdrPtr = objProperties->pObjAttributes;
	attribEndPtr = (cv_attrib_hdr *)((uint8_t *)attribHdrPtr + objProperties->objAttributesLength);
	found = FALSE;
	
	do
	{
		if (attribHdrPtr->attribType == CV_ATTRIB_TYPE_FLAGS)
		{
			found = TRUE;
			*flags = ((cv_attrib_flags *)(attribHdrPtr + 1));
			break;
		}
		attribHdrPtr = (cv_attrib_hdr *)((uint8_t *)(attribHdrPtr + 1) + attribHdrPtr->attribLen);
	} while (attribHdrPtr  < attribEndPtr); //Modified to fix Coverity error 
	if (!found)
	{
		retVal = CV_OBJECT_ATTRIBUTES_INVALID;
		goto err_exit;
	} else {
		/* set up storage atttibutes, if pointer supplied */
		if (attributes != NULL)
		{
			memset(attributes,0,sizeof(cv_obj_storage_attributes));
			if (objProperties->session->flags & CV_HAS_PRIVACY_KEY)
				attributes->hasPrivacyKey = 1;
			attributes->objectType = objPtr->objType;
			if (**flags & CV_ATTRIB_NVRAM_STORAGE)
				attributes->storageType = CV_STORAGE_TYPE_FLASH;
			else if (**flags & CV_ATTRIB_SC_STORAGE) 
				attributes->storageType = CV_STORAGE_TYPE_SMART_CARD;
			else if (**flags & CV_ATTRIB_CONTACTLESS_STORAGE)
				attributes->storageType = CV_STORAGE_TYPE_CONTACTLESS;
			else if (!(**flags & CV_ATTRIB_PERSISTENT))
				attributes->storageType = CV_STORAGE_TYPE_VOLATILE;
			else
				attributes->storageType = CV_STORAGE_TYPE_HARD_DRIVE;
		}
	}

err_exit:
	return retVal;
}


cv_status
cvPutObj(cv_obj_properties *objProperties, cv_obj_hdr *objPtr)
{
	/* this function saves the supplied object where indicated.  if a handle is supplied, this object */
	/* will replace an existing object.  if not, a new object is being created, so a handle will be */
	/* created. */
	cv_status retVal = CV_SUCCESS;
	cv_obj_handle objHandle = objProperties->objHandle;
	char objID[] = "cvobj_xxxxxxxx";
	uint32_t objCacheEntry = INVALID_CV_CACHE_ENTRY, L1CacheEntry = INVALID_CV_CACHE_ENTRY, L2CacheEntry = INVALID_CV_CACHE_ENTRY;
	uint8_t *objStoragePtr;
	cv_bool found = FALSE;
	cv_obj_handle level1DirHandle, level2DirHandle;
	cv_obj_handle objDirHandles[3];
	uint8_t *objDirPtrs[3];
	cv_attrib_flags *pFlags;
	uint32_t dir0Index, l1Index;
	cv_dir_page_level2 *dirL2Ptr = NULL;
	cv_dir_page_level1 *dirL1Ptr = NULL;
	cv_obj_storage_attributes objAttribs;
	uint32_t count, predictedCount;
	uint32_t objLen;
	uint32_t requestedHandle = objHandle;
	uint8_t *flashAllocationPtr = NULL;
	uint32_t i;
	uint32_t L1DirPageMarkedExists = 0, L2DirPageMarkedExists = 0;  /*, pageMarkedFull = 0;*/
	cv_hdotp_skip_table *pHdotpSkipTable = (cv_hdotp_skip_table *)((CHIP_IS_5882) ? CV_SKIP_TABLE_BUFFER : FP_CAPTURE_LARGE_HEAP_PTR);
	cv_bool doAntihammeringCheck = TRUE;

	/* check to see what type of storage required */
	/* first find flags word in object attribute list */
	/* Note: calling routine must supply all parameters for object.  this means that if an existing */
	/* object is being updated and the app didn't supply all parameters, which indicates existing object */
	/* parameters must be used, calling app must supply pointer to existing parameters too */
	/* set up object storage attributes */
	if ((retVal = cvFindObjAtribFlags(objProperties, objPtr, &pFlags, &objAttribs)) != CV_SUCCESS)
		goto err_exit;

	/* check to see if this is an existing object or not (or is a specific handle is requested) */
	if (!objHandle || objProperties->attribs.reqHandle)
	{
		/* no existing object, create handle. get random number (upper byte will be replaced by directory) */
		/* if specific handle requested, use that.  it may be determined later to not be available */
		if (!(objProperties->attribs.reqHandle))
		{
			if ((retVal = cvRandom(sizeof(cv_obj_handle), (uint8_t *)&objHandle)) != CV_SUCCESS)
				goto err_exit;
			objHandle &= HANDLE_VALUE_MASK;
			objProperties->objHandle = objHandle;
			requestedHandle = 0;
		}

		/* check to see if this is a volatile object */
		if (objAttribs.storageType == CV_STORAGE_TYPE_VOLATILE)
		{
			/* volatile object.  create entry in volatile object directory */
			if ((retVal = cvCreateVolatileDirEntry(objAttribs, objProperties, objPtr, objHandle)) != CV_SUCCESS)
				goto err_exit;
			goto obj_saved;
		}

		/* persistent object, get object cache entry to compose object in */
		cvObjCacheGetLRU(&objCacheEntry, &objStoragePtr);
		cvComposeObj(objProperties, objPtr, objStoragePtr);
		objDirHandles[1] = objDirHandles[0] = 0;
		objDirPtrs[1] = objDirPtrs[0] = NULL;

		/* see what kind of storage medium */
		if (objAttribs.storageType != CV_STORAGE_TYPE_HARD_DRIVE)
		{
			/* if NVRAM storage, need to allocate */
			if (objAttribs.storageType == CV_STORAGE_TYPE_FLASH)
			{
				/* check antihammering here, so flash allocation flash writes will be protected */
				if ((retVal = cvCheckAntiHammeringStatus()) != CV_SUCCESS)
					goto err_exit;
				doAntihammeringCheck = FALSE;
				if ((objDirPtrs[2] = cv_malloc_flash(cvGetWrappedSize(objPtr->objLen))) == NULL)
				{
					retVal = CV_FLASH_MEMORY_ALLOCATION_FAIL;
					goto err_exit;
				}
			}
			flashAllocationPtr = objDirPtrs[2];
			/* create and set up directory page 0 entry */
			if ((retVal = cvCreateDir0Entry(objAttribs, objProperties, objPtr, (uint8_t **)&objDirPtrs[0], objHandle, &dir0Index)) != CV_SUCCESS)
				goto err_exit;
			/* set up dir 0 entry pointer for cvUnwrapObj */
			objDirPtrs[1] = (uint8_t *)&CV_DIR_PAGE_0->dir0Objs[dir0Index];
		} else {
			/* persistent storage destination is host hard drive.  need to find entry in directory */
			/* structure. first look in directory page cache (level 2) */
			if (cvFindDirPageInL2Cache(requestedHandle, &dirL2Ptr, &L2CacheEntry))
			{
				/* if requesting a specific handle, need to make sure directory entry containing this */
				/* handle is availble.  if not, fail */
				if (requestedHandle && cvIsPageFull(dirL2Ptr->header.thisDirPage.hDirPage))
				{
					retVal = CV_INVALID_OBJECT_HANDLE;
					goto err_exit;
				}
				/* yes there is one available. create entry */
				cvCreateL2Dir(objAttribs, objProperties, &objHandle, NULL, 0, &L2CacheEntry, &dirL2Ptr);
				if (cvIsL2DirPageFull(dirL2Ptr))
				{
					cvSetPageFull(dirL2Ptr->header.thisDirPage.hDirPage);
				}
				/* corresponding L1 directory page should also be in cache, find it */
				if (!cvFindDirPageInL1Cache(objHandle, &dirL1Ptr, &L1CacheEntry))
				{
					/* L1 directory not found in cache, need to read it in */
					level1DirHandle = GET_L1_HANDLE_FROM_OBJ_HANDLE(objHandle);
					if ((retVal = cvGetL1L2Dir(objProperties, level1DirHandle, 0,	&L1CacheEntry, &dirL1Ptr, NULL, NULL)) != CV_SUCCESS)
					{
						/* failed to read in L1 dir page. */
						cvRemoveL2Entry(objHandle, dirL2Ptr);
						goto err_exit;
					}
				}
				/* got L1 and L2 directory pages */
				found = TRUE;
			}
			if (!found)
			{
				/* no available entries found in L2 cache, look at page map to find */
				/* available entry */
				/* if requesting a specific handle, need to make sure directory entry containing this */
				/* handle is available.  if not, fail */
				level2DirHandle = GET_L2_HANDLE_FROM_OBJ_HANDLE(requestedHandle);
				if (requestedHandle && cvIsPageFull(level2DirHandle))
				{
					retVal = CV_INVALID_OBJECT_HANDLE;
					goto err_exit;
				}
				/* if here and requestedHandle, then it must be available.  if no requestedHandle, try to find an L2 directory */
				if (requestedHandle || cvFindDirPageInPageMap(&level2DirHandle))
				{
					/* find entry in this page */
					found = TRUE;
					/* first get corresponding L1 directory */
					level1DirHandle = GET_L1_HANDLE_FROM_OBJ_HANDLE(level2DirHandle);
					/* test to see if it exists.  if so, read it in.  if not, create it */
					if (!cvPageExists(level1DirHandle))
					{
						/* no, create one in L1 cache */
						cvCreateL1Dir(level1DirHandle, level2DirHandle, &L1CacheEntry, &dirL1Ptr);
						cvSetPageExists(level1DirHandle);
						L1DirPageMarkedExists = level1DirHandle;
						/* if L1 didn't exist, need to create L2 also */
						l1Index = 0;
						dirL2Ptr = NULL;
						cvSetPageExists(level2DirHandle);
						L2DirPageMarkedExists = level2DirHandle;
					} else {
						/* yes, directory exists, but need to get from host.  also get L2 if necessary */
						if ((retVal = cvGetL1L2Dir(objProperties, level1DirHandle, level2DirHandle,
							&L1CacheEntry, &dirL1Ptr, &L2CacheEntry, &dirL2Ptr)) != CV_SUCCESS)
							goto err_exit;
						/* if L2 page exists, it would have been retrieved above.  if not, need to create it */

						if (!cvPageExists(level2DirHandle))
						{
							l1Index = cvGetAvailL1Index(dirL1Ptr);
							dirL2Ptr = NULL;
							cvSetPageExists(level2DirHandle);
							L2DirPageMarkedExists = level2DirHandle;
						}
					}
					/* fill in L2 directory entry for objectt */
					cvCreateL2Dir(objAttribs, objProperties, &objHandle, dirL1Ptr, l1Index, &L2CacheEntry, &dirL2Ptr);
				} else {
					retVal = CV_OBJECT_DIRECTORY_FULL;
					goto err_exit;
				}
			}
			/* set up L1 and L2 directory entries to write out, because they will need to be updated due to counter update */
			objDirPtrs[0] = (uint8_t *)dirL1Ptr;
			objDirPtrs[1] = (uint8_t *)dirL2Ptr;
			objDirHandles[0] = GET_L1_HANDLE_FROM_OBJ_HANDLE(objHandle);
			objDirHandles[1] = GET_L2_HANDLE_FROM_OBJ_HANDLE(objHandle);
			objProperties->objHandle = objHandle;
		}
	} else {
		/* object exists.  object and directory pages s/b in cache, if not get them */
		/* check to see if this is a volatile object */
		if (objAttribs.storageType == CV_STORAGE_TYPE_VOLATILE)
		{
			/* volatile object.  find entry in volatile object directory and update object */
			if ((retVal = cvUpdateVolatileDirEntry(objAttribs, objProperties, objPtr, objHandle)) != CV_SUCCESS)
				goto err_exit;
			goto obj_saved;
		}
		/* persistent object, check to see if in object cache */
		if (!cvFindObjInCache(objHandle, (cv_obj_hdr **)&objStoragePtr, &objCacheEntry))
			/* not is cache, get cache entry */
			cvObjCacheGetLRU(&objCacheEntry, &objStoragePtr);
		cvComposeObj(objProperties, objPtr, objStoragePtr);

		/* see what kind of storage medium */
		if (objAttribs.storageType != CV_STORAGE_TYPE_HARD_DRIVE)
		{
			/* find object in directory page 0 */
			if (!cvFindDir0Entry(objHandle, &dir0Index))
			{
				/* object not found */
				retVal = CV_INVALID_OBJECT_HANDLE;
				goto err_exit;
			}
			/* check antihammering here, so flash allocation flash writes will be protected */
			if ((retVal = cvCheckAntiHammeringStatus()) != CV_SUCCESS)
				goto err_exit;
			doAntihammeringCheck = FALSE;
			/* set up dir 0 entry pointer for cvUnwrapObj */
			objDirPtrs[1] = (uint8_t *)&CV_DIR_PAGE_0->dir0Objs[dir0Index];
			/* check to see if object needs to be reallocated */
			if (cvGetWrappedSize(objPtr->objLen) > CV_DIR_PAGE_0->dir0Objs[dir0Index].objLen)
			{
				/* yes, delete current object and allocate new flash */
				cv_free_flash(CV_DIR_PAGE_0->dir0Objs[dir0Index].pObj);
				if ((CV_DIR_PAGE_0->dir0Objs[dir0Index].pObj = cv_malloc_flash(cvGetWrappedSize(objPtr->objLen))) == NULL)
				{
					retVal = CV_FLASH_MEMORY_ALLOCATION_FAIL;
					goto err_exit;
				}
				CV_DIR_PAGE_0->dir0Objs[dir0Index].objLen = cvGetWrappedSize(objPtr->objLen);
			}
		} else {
			/* persistent storage destination is host hard drive. check to see if L1 and L2 dirs in cache */
			/* if not, read in */
			level1DirHandle = 0;
			level2DirHandle = 0;
			if (!cvFindDirPageInL1Cache(objHandle, &dirL1Ptr, &L1CacheEntry))
				level1DirHandle = GET_L1_HANDLE_FROM_OBJ_HANDLE(objHandle);
			if (!cvFindDirPageInL2Cache(objHandle, &dirL2Ptr, &L2CacheEntry))
				level2DirHandle = GET_L2_HANDLE_FROM_OBJ_HANDLE(objHandle);
			objDirPtrs[0] = (uint8_t *)dirL1Ptr;
			objDirPtrs[1] = (uint8_t *)dirL2Ptr;
			if (level1DirHandle || level2DirHandle)
				if ((retVal = cvGetL1L2Dir(objProperties, level1DirHandle, level2DirHandle, &L1CacheEntry, &dirL1Ptr, &L2CacheEntry, &dirL2Ptr )) != CV_SUCCESS)
					goto err_exit;
			/* set up L1 and L2 directory entries to write out, because they will need to be updated due to counter update */
			/* if retrieved directory, update pointer */
			if (level1DirHandle)
				objDirPtrs[0] = (uint8_t *)dirL1Ptr;
			if (level2DirHandle)
				objDirPtrs[1] = (uint8_t *)dirL2Ptr;
			objDirHandles[0] = GET_L1_HANDLE_FROM_OBJ_HANDLE(objHandle);
			objDirHandles[1] = GET_L2_HANDLE_FROM_OBJ_HANDLE(objHandle);
		}
	}
	/* object and directory pages (if host HD stored object) in caches.  do counter updates and write */
	/* also write mirrored objects and directory pages for directory page 0 objects */
	/* now check to see if anti-hammering will allow 2T usage (skip if rtn called above already) */
	if (doAntihammeringCheck)
		if ((retVal = cvCheckAntiHammeringStatus()) != CV_SUCCESS)
			goto err_exit;

	/* read skip tables */
	cvReadHDOTPSkipTable(pHdotpSkipTable);
	/* loop with 2T bump last, so that failure during process will cause retry of entire process */
	for (i=0;i<CV_MAX_2T_BUMP_FAIL;i++)
	{
		/* first do predictive bump to get next 2T value */
		if (get_predicted_monotonic_counter(&predictedCount) != CV_SUCCESS)
			goto err_exit;
		objLen = objPtr->objLen;
		/* set up to wrap object to prepare for persistent storage */
		objDirHandles[2] = objHandle;
		objDirPtrs[2] = objStoragePtr;
		if ((retVal = cvWrapObj(objProperties, objAttribs, objDirHandles, objDirPtrs, &objLen)) != CV_SUCCESS)
			continue;
		/* now send objects and directories */
		switch (objAttribs.storageType)
		{
		case CV_STORAGE_TYPE_FLASH:
			if ((retVal = cvPutFlashObj(&CV_DIR_PAGE_0->dir0Objs[dir0Index], CV_SECURE_IO_COMMAND_BUF)) != CV_SUCCESS)
			{
				printf("cvPutObj: cvPutFlashObj failed 0x%x\n",retVal);
				continue;
			}
			break;

		case CV_STORAGE_TYPE_SMART_CARD:
			sprintf(objID + 6,"%08x",objHandle);
			if ((retVal = scPutObj(sizeof(objID), objID, objProperties->PINLen, objProperties->PIN, 
				objLen, CV_SECURE_IO_COMMAND_BUF)) != CV_SUCCESS)
			{
				/* check to see why this failed, in order to determine if CV needs to handle */
				if (retVal == CV_PROMPT_FOR_SMART_CARD)
					CV_VOLATILE_DATA->CVInternalState |= CV_PENDING_WRITE_TO_SC;
				else if (retVal == CV_PROMPT_PIN)
					if (!(CV_VOLATILE_DATA->CVInternalState & CV_SESSION_UI_PROMPTS_SUPPRESSED))
						CV_VOLATILE_DATA->CVInternalState |= CV_WAITING_FOR_PIN_ENTRY;
				continue;
			}
			break;

		case CV_STORAGE_TYPE_CONTACTLESS:
			sprintf(objID + 6,"%08x",objHandle);
			if ((retVal = rfPutObj(sizeof(objID), objID, objProperties->PINLen, objProperties->PIN, 
				objLen, CV_SECURE_IO_COMMAND_BUF)) != CV_SUCCESS)
			{
				/* check to see why this failed, in order to determine if CV needs to handle */
				if (retVal == CV_PROMPT_FOR_CONTACTLESS)
					CV_VOLATILE_DATA->CVInternalState |= CV_PENDING_WRITE_TO_CONTACTLESS;
				else if (retVal == CV_PROMPT_PIN)
					if (!(CV_VOLATILE_DATA->CVInternalState & CV_SESSION_UI_PROMPTS_SUPPRESSED))
						CV_VOLATILE_DATA->CVInternalState |= CV_WAITING_FOR_PIN_ENTRY;
				continue;
			}
			break;

		case CV_STORAGE_TYPE_HARD_DRIVE:
			if ((retVal = cvPutHostObj((cv_host_storage_store_object *)CV_SECURE_IO_COMMAND_BUF)) != CV_SUCCESS)
				continue;
			break;
		case 0xffff:
			break;
		}

		/* if non-host storage object, need to write mirrored object to host HD and */
		/* wrap and write dir page 0 to flash */
		if (objAttribs.storageType != CV_STORAGE_TYPE_HARD_DRIVE)
		{
	#ifndef DEBUG_WO_HD_MIRRORING			
			cv_obj_storage_attributes objAttribsMirror = objAttribs;
			cv_obj_handle objDirHandlesMirror[3];

			/* first wrap object (still in object cache) with mirrored page value as handle and write to host HD */
			memcpy(objDirHandlesMirror, objDirHandles, sizeof(objDirHandlesMirror));
			objAttribsMirror.storageType = CV_STORAGE_TYPE_HARD_DRIVE;
			objDirHandlesMirror[2] |= CV_MIRRORED_OBJ_MASK;
			objAttribsMirror.objectType = objPtr->objType;
			objLen = objPtr->objLen;
			if ((retVal = cvWrapObj(objProperties, objAttribsMirror, objDirHandlesMirror, objDirPtrs, &objLen)) != CV_SUCCESS)
				continue;
			if ((retVal = cvPutHostObj((cv_host_storage_store_object *)CV_SECURE_IO_COMMAND_BUF)) != CV_SUCCESS)
				continue;
			if ((retVal = cvWriteDirectoryPage0(TRUE)) != CV_SUCCESS)
				continue;
	#else			
			if ((retVal = cvWriteDirectoryPage0(FALSE)) != CV_SUCCESS)
			{
				printf("cvPutObj: cvWriteDirectoryPage0 failed 0x%x\n",retVal);
				continue;
			}
	#endif			
		}
		
		/* now wrap and write top level dir to flash */
#ifndef DEBUG_WO_HD_MIRRORING	
		if ((retVal = cvWriteTopLevelDir(TRUE)) != CV_SUCCESS)
			continue;
#else	
		if ((retVal = cvWriteTopLevelDir(FALSE)) != CV_SUCCESS)
		{
			printf("cvPutObj: cvWriteTopLevelDir failed 0x%x\n",retVal);
			continue;
		}
#endif	
		/* write out HDOTP skip table */
		if ((retVal = cvWriteHDOTPSkipTable()) != CV_SUCCESS)
		{
			printf("cvPutObj: cvWriteHDOTPSkipTable failed 0x%x\n",retVal);
			continue;
		}
		/* now try to bump 2T */
		if ((retVal = bump_monotonic_counter(&count)) != CV_SUCCESS)
		{
			printf("cvPutObj: bump_monotonic_counter failed 0x%x\n",retVal);
			continue;
		}
		/* if 2T succeeded, make sure matches predicted count */
		if (count == predictedCount)
		{
			CV_VOLATILE_DATA->retryWrites = 0;
			break;
		}else{
			printf("cvPutObj: count 0x%x  predictedCount 0x%x\n",count, predictedCount);
		}
	}
	/* if fail to bump count successfully, indicate failure here */
	if (i == CV_MAX_2T_BUMP_FAIL && count != predictedCount)
		retVal = CV_MONOTONIC_COUNTER_FAIL;
	if (retVal != CV_SUCCESS)
		goto err_exit;
	/* update the cache entry which was used to be the MRU entry */
	CV_VOLATILE_DATA->objCache[objCacheEntry] = objHandle;
	cvUpdateObjCacheMRU(objCacheEntry);
	if (L1CacheEntry != INVALID_CV_CACHE_ENTRY)
	{
		CV_VOLATILE_DATA->level1DirCache[L1CacheEntry] = objDirHandles[0];
		cvUpdateLevel1CacheMRU(L1CacheEntry);
	}
	if (L2CacheEntry != INVALID_CV_CACHE_ENTRY)
	{
		CV_VOLATILE_DATA->level2DirCache[L2CacheEntry] = objDirHandles[1];
		cvUpdateLevel2CacheMRU(L2CacheEntry);
	}
err_exit:
obj_saved:
	/* check to see if error occurred and need to undo any actions */
    if (retVal != CV_SUCCESS)
	{
		/* check to see if flash was allocated */
		if (flashAllocationPtr != NULL)
		{
			/* yes, free allocated memory */
			cv_free_flash(flashAllocationPtr);
			CV_DIR_PAGE_0->dir0Objs[dir0Index].hObj = 0;
		}
		/* check to see if host hard drive object.  if so, just flush caches to undo any changes */
		if (objAttribs.storageType == CV_STORAGE_TYPE_HARD_DRIVE)
			cvFlushAllCaches();
		if (L1DirPageMarkedExists)
			cvSetPageAbsent(L1DirPageMarkedExists);
		if (L2DirPageMarkedExists)
			cvSetPageAbsent(L2DirPageMarkedExists);
		/*Removed the below dead code to fix coverity issue */
		/*
		if (pageMarkedFull)
			cvSetPageAvail(pageMarkedFull);
		*/
	}
	/* invalidate HDOTP skip table here so that buffer (FP capture buffer) can be reused */
	cvInvalidateHDOTPSkipTable();
	return retVal;
}

cv_status
cvDelObj(cv_obj_properties *objProperties, cv_bool suppressExtDeviceObjDel)
{
	/* this function deletes the indicated object */
	cv_status retVal = CV_SUCCESS;
	cv_obj_handle objHandle = objProperties->objHandle;
	uint32_t i,j;
	char objID[] = "cvobj_xxxxxxxx";
	cv_obj_handle objDirHandles[3] = {0,0,0};
	uint8_t *objDirPtrs[3], *objDirPtrsOut[3];
	uint32_t count, predictedCount;
	cv_obj_storage_attributes objAttribs;
	uint32_t objLen = 0;
	cv_dir_page_entry_obj_flash flashEntry;
	uint32_t L1CacheEntry = INVALID_CV_CACHE_ENTRY, L2CacheEntry = INVALID_CV_CACHE_ENTRY;
	cv_obj_hdr *objPtr;
	cv_hdotp_skip_table *pHdotpSkipTable = (cv_hdotp_skip_table *)((CHIP_IS_5882) ? CV_SKIP_TABLE_BUFFER : FP_CAPTURE_LARGE_HEAP_PTR);
	cv_bool doAntihammeringCheck = TRUE;
	cv_obj_type objType;

	/* first flush object from cache */
	cvFlushCacheEntry(objHandle);

	/* check to see if object is in directory page 0 */
	if (GET_DIR_PAGE(objHandle) == 0)
	{
		/* yes, first search volatile object directory */
		if (cvFindVolatileDirEntry(objHandle, &i))
		{
			/* object found, delete it */
			objPtr = (cv_obj_hdr *)CV_VOLATILE_OBJ_DIR->volObjs[i].pObj;
			objType = objPtr->objType;
			cv_free(CV_VOLATILE_OBJ_DIR->volObjs[i].pObj, objPtr->objLen);
			CV_VOLATILE_OBJ_DIR->volObjs[i].hObj = 0;
			/* if auth session object, may also have flash object that needs to be deleted, so just continue */
			if (!(objType == CV_TYPE_AUTH_SESSION && cvFindDir0Entry(objHandle, &i)))
				goto obj_found;
		}
		/* now search directory page 0 */
		if (cvFindDir0Entry(objHandle, &i))
		{
			/* object found, determine where object resident.  then delete */
			objDirPtrs[1] = (uint8_t *)&CV_DIR_PAGE_0->dir0Objs[i];
			switch (CV_DIR_PAGE_0->dir0Objs[i].attributes.storageType)
			{
			case CV_STORAGE_TYPE_FLASH:
				/* check antihammering here, so flash deallocation flash writes will be protected */
				if ((retVal = cvCheckAntiHammeringStatus()) != CV_SUCCESS)
					goto err_exit;
				doAntihammeringCheck = FALSE;
				cv_free_flash(CV_DIR_PAGE_0->dir0Objs[i].pObj);
				objAttribs.storageType = CV_STORAGE_TYPE_FLASH;
			break;
			case CV_STORAGE_TYPE_SMART_CARD:
				if (!suppressExtDeviceObjDel)
				{
					sprintf(objID + 6,"%08x",objHandle);
					if ((retVal = scDelObj(sizeof(objID), objID, objProperties->PINLen, objProperties->PIN)) != CV_SUCCESS)
							goto err_exit;
				}
				objAttribs.storageType = CV_STORAGE_TYPE_SMART_CARD;
				break;
			case CV_STORAGE_TYPE_CONTACTLESS:
				if (!suppressExtDeviceObjDel)
				{
					sprintf(objID + 6,"%08x",objHandle);
					if ((retVal = rfDelObj(sizeof(objID), objID, objProperties->PINLen, objProperties->PIN)) != CV_SUCCESS)
							goto err_exit;
				}
				objAttribs.storageType = CV_STORAGE_TYPE_CONTACTLESS;
				break;
			case 0xffff:
				break;
			}
			CV_DIR_PAGE_0->dir0Objs[i].hObj = 0;
		} else {
			retVal = CV_INVALID_OBJECT_HANDLE;
			goto err_exit;
		}
	} else {
		/* not directory page 0, which means object stored on host hard drive */
		/* when object deleted, it will be necessary to modify the L2 directory page to remove the entry */
		/* and bump the counts on L1 and top level directory pages, so must find L1 and L2 pages in cache or read in */
		/* first search object cache and invalidate, if found */

		objAttribs.storageType = CV_STORAGE_TYPE_HARD_DRIVE;
		/* first search directory page caches for directory pages in hierarchy above this object */
		objDirHandles[0] = objDirHandles[1] = objDirHandles[2] = 0;
		if (!cvFindDirPageInL1Cache(objHandle, (cv_dir_page_level1 **)&objDirPtrs[0], &L1CacheEntry))
		{
			/* not in dir page cache, need to read in.  get slot in dir page cache */
			cvLevel1DirCacheGetLRU(&L1CacheEntry, &objDirPtrs[0]);
			objDirHandles[0] = GET_L1_HANDLE_FROM_OBJ_HANDLE(objHandle);
		}
		if (!cvFindDirPageInL2Cache(objHandle, (cv_dir_page_level2 **)&objDirPtrs[1], &L2CacheEntry))
		{
			/* not in dir page cache, need to read in.  get slot in dir page cache */
			cvLevel2DirCacheGetLRU(&L2CacheEntry, &objDirPtrs[1]);
			objDirHandles[1] = GET_L2_HANDLE_FROM_OBJ_HANDLE(objHandle);
		}
		/* save cache pointers */
		objDirPtrsOut[0] = objDirPtrs[0];
		objDirPtrsOut[1] = objDirPtrs[1];
		/* now delete object and retrieve any directory pages not in cache (in order to remove object entry) */
		cvSetPageAvail(objHandle);
		cvDelHostObj(objHandle, (cv_host_storage_delete_object *)CV_SECURE_IO_COMMAND_BUF);
		/* if one or both directory pages not in cache, retrieve from host */
		if (objDirHandles[0] || objDirHandles[1])
		{
			if ((retVal = cvGetHostObj(&objDirHandles[0], &objDirPtrs[0], &objDirPtrsOut[0])) != CV_SUCCESS)
				goto err_exit;
			/* now unwrap directory pages retrieved, if any */
			objAttribs.storageType = CV_STORAGE_TYPE_HARD_DRIVE;
			if ((retVal = cvUnwrapObj(objProperties, objAttribs, &objDirHandles[0], &objDirPtrs[0], &objDirPtrsOut[0])) != CV_SUCCESS)
				goto err_exit;
		}
		/* restore cache pointers */
		objDirPtrs[0] = objDirPtrsOut[0];
		objDirPtrs[1] = objDirPtrsOut[1];
		/* locate entry for object in L2 dir page and indicate available */
		if (!cvFindObjEntryInL2Dir(objHandle, (cv_dir_page_level2 *)objDirPtrs[1], &i))
		{
			retVal = CV_INVALID_OBJECT_HANDLE;
			goto err_exit;
		}
		/* clear entry associated with object being deleted */
		((cv_dir_page_level2 *)objDirPtrs[1])->dirEntries[i].hObj = 0;
	}
	/* now check to see if anti-hammering will allow 2T usage (unless already done above) */
	if (doAntihammeringCheck)
		if ((retVal = cvCheckAntiHammeringStatus()) != CV_SUCCESS)
			goto err_exit;

	/* read skip tables */
	cvReadHDOTPSkipTable(pHdotpSkipTable);
	/* loop with 2T bump last, so that failure during process will cause retry of entire process */
	for (j=0;j<CV_MAX_2T_BUMP_FAIL;j++)
	{
		/* first do predictive bump to get next 2T value */
		if (get_predicted_monotonic_counter(&predictedCount) != CV_SUCCESS)
			goto err_exit;
		/* first check to see if host hard drive object */
		if (objAttribs.storageType == CV_STORAGE_TYPE_HARD_DRIVE)
		{
			objAttribs.storageType = CV_STORAGE_TYPE_HARD_DRIVE;
			/* set up directory handles for writing directories to hard disk */
			objDirHandles[0] = GET_L1_HANDLE_FROM_OBJ_HANDLE(objHandle);
			objDirHandles[1] = GET_L2_HANDLE_FROM_OBJ_HANDLE(objHandle);
			if ((retVal = cvWrapObj(objProperties, objAttribs, objDirHandles, objDirPtrs, &objLen)) != CV_SUCCESS)
				continue;
			if ((retVal = cvPutHostObj((cv_host_storage_store_object *)CV_SECURE_IO_COMMAND_BUF)) != CV_SUCCESS)
				continue;
		} else {
			/* if non-host storage object, need to delete mirrored object on host HD and */
			/* wrap and write dir page 0 to flash */
#ifndef DEBUG_WO_HD_MIRRORING
			cvDelHostObj(objHandle | CV_MIRRORED_OBJ_MASK, (cv_host_storage_delete_object *)CV_SECURE_IO_COMMAND_BUF);
#endif
			/* now wrap and write dir page 0 to flash */
			objAttribs.storageType = CV_STORAGE_TYPE_FLASH_NON_OBJ;
			objDirPtrs[2] = (uint8_t *)CV_DIR_PAGE_0;
			objLen = sizeof(cv_dir_page_0);
			if ((retVal = cvWrapObj(objProperties, objAttribs, objDirHandles, objDirPtrs, &objLen)) != CV_SUCCESS)
				continue;
			flashEntry.pObj = CV_DIRECTORY_PAGE_0_FLASH_ADDR;
			flashEntry.objLen = objLen;
			if ((retVal = cvPutFlashObj(&flashEntry, CV_SECURE_IO_COMMAND_BUF)) != CV_SUCCESS)
				continue;
			/* now mirror dir page 0 to host HD as well */
			objAttribs.storageType = CV_STORAGE_TYPE_HARD_DRIVE_NON_OBJ;
			objDirPtrs[2] = (uint8_t *)CV_DIR_PAGE_0;
			objDirHandles[2] = CV_MIRRORED_DIR_0;
			objLen = sizeof(cv_dir_page_0);
			if ((retVal = cvWrapObj(objProperties, objAttribs, objDirHandles, objDirPtrs, &objLen)) != CV_SUCCESS)
				continue;
#ifndef DEBUG_WO_HD_MIRRORING						
			if ((retVal = cvPutHostObj((cv_host_storage_store_object *)CV_SECURE_IO_COMMAND_BUF)) != CV_SUCCESS)
				continue;
			if ((retVal = cvWriteDirectoryPage0(TRUE)) != CV_SUCCESS)
				continue;
#else
			if ((retVal = cvWriteDirectoryPage0(FALSE)) != CV_SUCCESS)
				continue;
#endif			
		}
#ifndef DEBUG_WO_HD_MIRRORING	
		if ((retVal = cvWriteTopLevelDir(TRUE)) != CV_SUCCESS)
			continue;
#else	
		if ((retVal = cvWriteTopLevelDir(FALSE)) != CV_SUCCESS)
			continue;
#endif				
		/* write out HDOTP skip table */
		if ((retVal = cvWriteHDOTPSkipTable()) != CV_SUCCESS)
			continue;
		/* now try to bump 2T */
		if ((retVal = bump_monotonic_counter(&count)) != CV_SUCCESS)
			continue;
		/* if 2T succeeded, make sure matches predicted count */
		if (count == predictedCount)
		{
			CV_VOLATILE_DATA->retryWrites = 0;
			break;
		}
	}
	/* if fail to bump count successfully, indicate failure here */
	if (j == CV_MAX_2T_BUMP_FAIL && count != predictedCount)
		retVal = CV_MONOTONIC_COUNTER_FAIL;
	if (retVal != CV_SUCCESS)
		goto err_exit;
	/* put these directory page handles into cache list (at MRU location) and update MRU list */
	/* if any were retrieved */
	/* also update cache lists for directory pages, if any were retrieved */
	if (L1CacheEntry != INVALID_CV_CACHE_ENTRY)
	{
		CV_VOLATILE_DATA->level1DirCache[L1CacheEntry] = objDirHandles[0];
		cvUpdateLevel1CacheMRU(L1CacheEntry);
	}
	if (L2CacheEntry != INVALID_CV_CACHE_ENTRY)
	{
		CV_VOLATILE_DATA->level1DirCache[L2CacheEntry] = objDirHandles[1];
		cvUpdateLevel2CacheMRU(L2CacheEntry);
	}

err_exit:
obj_found:
	/* invalidate HDOTP skip table here so that buffer (FP capture buffer) can be reused */
	cvInvalidateHDOTPSkipTable();
	return retVal;
}

cv_status
cvEnumObj(cv_obj_properties *objProperties, cv_obj_type objType, uint32_t *bufLen, cv_obj_handle *handleBuf, cv_bool resume)
{
	/* this routine enumerates all objects in CV that match user/app ID provided (or all objects */
	/* if ID is 0).  If not all objects will fit in buffer, a status is returned indicating this */
	/* case.  If routine is then called with resume == TRUE, object enumeration will continue where */
	/* it left off */
	uint32_t bufIndex = 0;
	uint32_t i, j;
	cv_bool skippingObjs = FALSE, enumerateAll = TRUE;
	cv_obj_handle nextHandle = 0;
	cv_status retVal = CV_SUCCESS;
	cv_obj_handle level2DirHandle, level1DirHandle;
	uint32_t L2CacheEntry = INVALID_CV_CACHE_ENTRY, L1CacheEntry = INVALID_CV_CACHE_ENTRY;
	cv_dir_page_level2 *dirL2Ptr;
	cv_dir_page_level1 *dirL1Ptr;
	uint32_t count, predictedCount;
	uint32_t index;

	/* if this is a resume, get last obj handle enumerated from volatile data */
	if (resume)
	{
		/* check to see if handle is non-null */
		if (objProperties->session->nextEnumeratedHandle)
		{
			nextHandle = objProperties->session->nextEnumeratedHandle;
			skippingObjs = TRUE;
		}
	}
	/* check to see if enumerating all objects (ie, user/app ID is 0) */
	for (i=0;i<SHA1_LEN;i++)
	{
		if (objProperties->session->appUserID[i] != 0)
		{
			enumerateAll = FALSE;
			break;
		}
	}

	/* don't need to go through directory page 0 objects, if this is a resumed enumeration */
	/* and the next object handle was not in directory page 0 */
	if (!skippingObjs || (skippingObjs && GET_DIR_PAGE(nextHandle) == 0))
	{
		/* first enumerate objects in volatile directory */
		for (i=0;i<MAX_CV_NUM_VOLATILE_OBJS;i++)
		{
			if (CV_VOLATILE_OBJ_DIR->volObjs[i].hObj != 0)
			{
				/* check user/app ID to see if this object s/b enumerated */
				if (!enumerateAll && memcmp(objProperties->session->appUserID, CV_VOLATILE_OBJ_DIR->volObjs[i].appUserID, SHA1_LEN))
					continue;
				/* check object type to see if this object s/b enumerated */
				if (objType && ((cv_obj_hdr *)CV_VOLATILE_OBJ_DIR->volObjs[i].pObj)->objType != objType)
					continue;
				/* check to see if skipping objects */
				if (skippingObjs)
				{
					/* check to see if we found the next object to be enumerated */
					if (CV_VOLATILE_OBJ_DIR->volObjs[i].hObj == nextHandle)
					{
						skippingObjs = FALSE;
					} else
						continue;
				}
				/* check to see if buffer provided is full */
				if (bufIndex*sizeof(uint32_t) >= *bufLen)
				{
					objProperties->session->nextEnumeratedHandle = CV_VOLATILE_OBJ_DIR->volObjs[i].hObj;
					/* yes, return with incomplete status */
					retVal = CV_ENUMERATION_BUFFER_FULL;
					goto err_exit;
				}
				handleBuf[bufIndex++] = CV_VOLATILE_OBJ_DIR->volObjs[i].hObj;
			}
		}
		/* continue enumerating objects in directory page 0 */
		/* first check flash heap to remove any allocated blocks not associated with CV objects */
//		cvCheckFlashHeapConsistency();
		/* also check to see if any dir 0 entries point to invalid (deallocated) blocks.  if so, clear entry and update dir 0 in flash */
		if (!cvIsDir0Consistent())
		{
			cv_hdotp_skip_table *pHdotpSkipTable = (cv_hdotp_skip_table *)((CHIP_IS_5882) ? CV_SKIP_TABLE_BUFFER : FP_CAPTURE_LARGE_HEAP_PTR);

			/* now check to see if anti-hammering will allow 2T usage */
			if ((retVal = cvCheckAntiHammeringStatus()) != CV_SUCCESS)
				goto err_exit;
			/* now bump counter, so that dir page 0 will get new value */
			/* loop with 2T bump last, so that failure during process will cause retry of entire process */

			/* read skip tables */
			cvReadHDOTPSkipTable(pHdotpSkipTable);
			for (j=0;j<CV_MAX_2T_BUMP_FAIL;j++)
			{
				/* first do predictive bump to get next 2T value */
				if (get_predicted_monotonic_counter(&predictedCount) != CV_SUCCESS)
					goto err_exit;
				/* write out updated dir page 0 */
				if ((retVal = cvWriteDirectoryPage0(TRUE)) != CV_SUCCESS)
					continue;
				/* write out top level directory */
				if ((retVal = cvWriteTopLevelDir(TRUE)) != CV_SUCCESS)
					continue;
				/* write out HDOTP skip table */
				if ((retVal = cvWriteHDOTPSkipTable()) != CV_SUCCESS)
					continue;
				/* now try to bump 2T */
				if ((retVal = bump_monotonic_counter(&count)) != CV_SUCCESS)
					continue;
				/* if 2T succeeded, make sure matches predicted count */
				if (count == predictedCount)
				{
					/* successful, zero retry flags */
					CV_VOLATILE_DATA->retryWrites = 0;
					break;
				}
			}
			/* check to see if loop terminated successfully */
			if (retVal == CV_SUCCESS && (j == CV_MAX_2T_BUMP_FAIL))
				/* count never matched predicted value */
				retVal = CV_MONOTONIC_COUNTER_FAIL;
			if (retVal != CV_SUCCESS)
				goto err_exit;
		}

		for (i=0;i<MAX_DIR_PAGE_0_ENTRIES;i++)
		{
			if (CV_DIR_PAGE_0->dir0Objs[i].hObj != 0)
			{
				/* check user/app ID to see if this object s/b enumerated */
				if (!enumerateAll && memcmp(objProperties->session->appUserID, CV_DIR_PAGE_0->dir0Objs[i].appUserID, SHA1_LEN))
					continue;
				/* check object type to see if this object s/b enumerated */
				if (objType && CV_DIR_PAGE_0->dir0Objs[i].attributes.objectType != objType)
					continue;
				/* check to see if skipping objects */
				if (skippingObjs)
				{
					/* check to see if we found the next object to be enumerated */
					if (CV_DIR_PAGE_0->dir0Objs[i].hObj == nextHandle)
					{
						skippingObjs = FALSE;
					} else
						continue;
				}
				/* check to see if buffer provided is full */
				if (bufIndex*sizeof(uint32_t) >= *bufLen)
				{
					objProperties->session->nextEnumeratedHandle = CV_DIR_PAGE_0->dir0Objs[i].hObj;
					/* yes, return with incomplete status */
					retVal = CV_ENUMERATION_BUFFER_FULL;
					goto err_exit;
				}
				/* if this is an auth session object, check to see if it also exists mirrored in volatile */
				/* memory (and would be enumerated there).  if so, skip here, to avoid duplicate object handles */
				if (CV_DIR_PAGE_0->dir0Objs[i].attributes.objectType == CV_TYPE_AUTH_SESSION)
					if (cvFindVolatileDirEntry(CV_DIR_PAGE_0->dir0Objs[i].hObj, &index))
						if (enumerateAll || !memcmp(objProperties->session->appUserID, CV_VOLATILE_OBJ_DIR->volObjs[index].appUserID, SHA1_LEN))
							continue;
				handleBuf[bufIndex++] = CV_DIR_PAGE_0->dir0Objs[i].hObj;
			}
		}
	}
	/* now enumerate host objects by retrieving all level 2 directory pages and enumerating objects in each */
	for (i=MIN_L2_DIR_PAGE;i<=MAX_L2_DIR_PAGE;i++)
	{
		dirL2Ptr = NULL;
		level2DirHandle = GET_HANDLE_FROM_PAGE(i);
		level1DirHandle = cvGetLevel1DirPageHandle(level2DirHandle);
		/* only request pages if exist */
		if (cvPageExists(level2DirHandle))
		{
			/* get level 1 dir page too, for anti-replay check */
			if ((retVal = cvGetL1L2Dir(objProperties, level1DirHandle, level2DirHandle,  &L1CacheEntry, &dirL1Ptr,  &L2CacheEntry, &dirL2Ptr)) != CV_SUCCESS)
			{
				/* if error occurs enumerating host storage objects, just ignore, but don't continue */
				/* enumeration of host storage objects */
				retVal = CV_SUCCESS;
				break;
			}
		}

		/* if it found this page ok, enumerate objects in it */
		if (dirL2Ptr != NULL)
		{
			for (j=0;j<MAX_CV_ENTRIES_PER_DIR_PAGE;j++)
			{
				if (dirL2Ptr->dirEntries[j].hObj)
				{
					/* check user/app ID to see if this object s/b enumerated */
					if (!enumerateAll && memcmp(objProperties->session->appUserID, dirL2Ptr->dirEntries[j].appUserID, SHA1_LEN))
						continue;
					/* check object type to see if this object s/b enumerated */
					if (objType && dirL2Ptr->dirEntries[j].attributes.objectType != objType)
						continue;
					/* check to see if skipping objects */
					if (skippingObjs)
					{
						/* check to see if we found the next object to be enumerated */
						if (dirL2Ptr->dirEntries[j].hObj == nextHandle)
						{
							skippingObjs = FALSE;
						} else
							continue;
					}
					/* check to see if buffer provided is full */
					if (bufIndex*sizeof(uint32_t) >= *bufLen)
					{
						objProperties->session->nextEnumeratedHandle = dirL2Ptr->dirEntries[j].hObj;
						/* yes, return with incomplete status */
						retVal = CV_ENUMERATION_BUFFER_FULL;
						goto err_exit;
					}
					handleBuf[bufIndex++] = dirL2Ptr->dirEntries[j].hObj;
				}
			}
		}
	}
err_exit:
	*bufLen = bufIndex*sizeof(uint32_t);
	/* invalidate HDOTP skip table here so that buffer (FP capture buffer) can be reused */
	cvInvalidateHDOTPSkipTable();
	return retVal;
}



