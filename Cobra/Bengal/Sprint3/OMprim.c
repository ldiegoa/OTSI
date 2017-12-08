code/asic.ss/pv.p/matrix.p/feature.p/omVlanTagRam.c

/**PROC+**********************************************************************
 * Name:      asicWriteOmVidRemapEnable
 *
 * Purpose:   Enable VID Remap for a port
 *
 * Params:    PPORT_t, boolean
 *
 * Returns:   bool
 *
 **PROC-**********************************************************************/

bool asicWriteOmVidRemapEnable(PPORT_t pport, boolean enable)
{
   IPORT_t       iport;
   uint64_t      enableMap;
   CHIP_PORT_t   chipPort;

   iport = PPORT_TO_IPORT(pport);

   chipPort = portNum_slotIPort_to_chipPort(IPORT_TO_SLOT_IPORT(iport));

   /* Enable/Disable the OM of the specified chip port.
    */

   asicReadMem(0, NEO_OM_FP_VID_REMAP_EN, 0, (uint32_t *)&enableMap);
   if (enable)
   {
       enableMap |=  (((uint64_t)1) << chipPort);
   }
   else
   {
       enableMap &= ~(((uint64_t)1) << chipPort);
   }
   asicWriteMem(0, NEO_OM_FP_VID_REMAP_EN, 0, (uint32_t *)&enableMap);

   return TRUE;
}

/**PROC+**********************************************************************
 * Name:      asicReadOmVidRemapEnable
 *
 :   Returns VID remap status of  a given pport
 *
 * Params:    PPORT_t, boolean
 *
 * Returns:   bool
 *
 **PROC-**********************************************************************/

bool asicReadOmVidRemapEnable(PPORT_t pport)
{
   IPORT_t       iport;
   uint64_t      enableMap;
   CHIP_PORT_t   chipPort;

   iport = PPORT_TO_IPORT(pport);

   chipPort = portNum_slotIPort_to_chipPort(IPORT_TO_SLOT_IPORT(iport));

   /* Enable/Disable the OM of the specified chip port. */

   asicReadMem(0, NEO_OM_FP_VID_REMAP_EN, 0, (uint32_t *)&enableMap);
   if ((enableMap) & (((uint64_t) 1) << chipPort))
   {
       return TRUE;
   }

   return FALSE;
}

/**PROC+**********************************************************************
 * Name:      asicReadOmVidRemapSelect
 *
 * Purpose:   Returns VID remap select status of  a given pport
 *
 * Params:    PPORT_t 
 *
 * Returns:   bool
 *
 **PROC-**********************************************************************/
bool asicReadOmVidRemapSelect(PPORT_t pport)
{
   IPORT_t       iport;
   uint64_t      enableMap;
   CHIP_PORT_t   chipPort;

   iport = PPORT_TO_IPORT(pport);

   chipPort = portNum_slotIPort_to_chipPort(IPORT_TO_SLOT_IPORT(iport));

   /*  Enable/Disable the OM of the specified chip port. */

   asicReadMem(0, NEO_OM_FP_VID_REMAP_SEL, 0, (uint32_t *)&enableMap);
   if ((enableMap) & (((uint64_t) 1) << chipPort))
   {
      return TRUE;
   }
   return FALSE;

/**PROC+**********************************************************************
 * Name:      asicWriteOmVlanTagRamwithIport
 *
 * Purpose:   Write to the Outbound Memory VLAN TAG register
 *
 * Params:    iport: internal port 
 *            vid:   VLAN ID
 *            value: What you want to write to register
 *
 * Returns:   none
 *
 **PROC-**********************************************************************/
void asicWriteOmVlanTagRamWithIport(IPORT_t iport, VID_t vid, uint32_t value)
{
   CHIP_PORT_t chipPort;
   uint32_t fpGroupNum, fpGroupPortNum;
   uint32_t *directAccessReg;
   uint32_t readVal;
   int i = 0;

   /* convert iport to chipport */
   chipPort = portNum_slotIPort_to_chipPort(IPORT_TO_SLOT_IPORT(iport));
   fpGroupNum  = omPortToFPGroup[chipPort];
   fpGroupPortNum = omPortToFPPORTGROUP[chipPort];

   /* We do not want the Foce tag bit to be set on VID 4095 */
   if(INTERNAL_VID == vid) 
   {
      value = OM_VLAN_UNTAGGED_VALUE;
   }

   /* Get the omVlanTagRamReg address. */
   directAccessReg = asicGetRegMemAddr(0, omVlanTagRamReg[fpGroupNum], 0); 
   ASSERT(directAccessReg);

   /* Register Address is (Front Plane group * 0x4000) + VID * 4 */
   directAccessReg += (fpGroupPortNum * 0x1000) + vid;

   /* An ASIC bug in Neo can convert hardware writes to this ASIC block into
    * hardware reads when packets are flowing through the system. The code
    * below repeats the attempt to write until the read returns the expected
    * value, up to a limit.
    */
   do
   {
      asicDirectMemWrite32(0, directAccessReg, value);
      asicDirectMemRead32(0, directAccessReg, &readVal);
      i++;
   }
   while (((readVal & ~OM_VLAN_PARITY) != value) && (i < MAX_OM_WRITE_ATTEMPTS));

   if ((readVal & ~OM_VLAN_PARITY) != value)
   {
      CHIP_PORT_t chipPortIter;
      uint64_t fpgroup = 0;
      uint64_t macFeed = 0;
      uint64_t macFeedOrig = 0;
      char log[80] = { 0 };

      /* get the ports in the group and subtract DMA and conduit ports */
      for (chipPortIter = 0; chipPortIter <= MAX_CHIP_PORTS; chipPortIter++)
      {
         if (omPortToFPGroup[chipPortIter] == fpGroupNum)
         {
            CHIP_PORT_ROLE_t role = bsp_chipPort_role(chipPort);

            /* we do not want to shut down DMA or conduit ports */
            if ((ROLE_NC != role) && (ROLE_CPU != role) && (ROLE_CONDUIT != role))
            {
               fpgroup |= (((uint64_t)1) << chipPortIter);
            }
         }
      }

      /* read the register for those ports */
      asicReadMem(0, NEO_OM_FP_MISC_MAC_FEED_OPERATE, 0, (uint32_t *)&macFeedOrig);

      /* shutdown those ports */
      macFeed = macFeedOrig & (~fpgroup);
      asicWriteMem(0, NEO_OM_FP_MISC_MAC_FEED_OPERATE, 0, (uint32_t *)&macFeed);

      /* redo the write-and-check loop */
      i = 0;
      do
      {
         asicDirectMemWrite32(0, directAccessReg, value);
         asicDirectMemRead32(0, directAccessReg, &readVal);
         i++;
      }
      while (((readVal & ~OM_VLAN_PARITY) != value) && (i < MAX_OM_WRITE_ATTEMPTS));

      /* re-enable the ports as soon as possible */
      asicWriteMem(0, NEO_OM_FP_MISC_MAC_FEED_OPERATE, 0, (uint32_t *)&macFeedOrig);

      /* on failed completion, fatal except */
      if ((readVal & ~OM_VLAN_PARITY) != value)
      {
         FATAL_EXCEPTION("Unable to write Vlan tag setting");
      }

      /* log that we may have dropped some packets */
      snprintf(log, sizeof(log), GET_RMON_EVENT(RMON_ADDRMGR_VLAN_PKT_DROP), vid);
      rmon_log_event(RMON_ADDRMGR_VLAN_PKT_DROP, log);
   }

} /* asicWriteOmVlanTagRamWithIport */


/**PROC+**********************************************************************
 * Name:      asicWriteOmVlanTagRam
 *
 * Purpose:   Wrapper function around asicWriteOmVlanTagRamWithIport
 *
 * Params:    pport: physical port 
 *            vid:   VLAN ID
 *            value: What you want to write to register
 *
 * Returns:   none
 *
 **PROC-**********************************************************************/
void asicWriteOmVlanTagRam(PPORT_t pport, VID_t vid, uint32_t value)
{
   asicWriteOmVlanTagRamWithIport(PPORT_TO_IPORT(pport), vid, value);
}

/**PROC+**********************************************************************
 * Name:      asicReadOmVlanTagRamwithIport
 *
 * Purpose:   Read the Outbound Memory VLAN TAG register
 *
 * Params:    iport: internal port 
 *            vid:   VLAN ID
 *            value: address of a variable where the read value will be stored
 *
 * Returns:   none
 *
 **PROC-**********************************************************************/
void asicReadOmVlanTagRamWithIport(IPORT_t iport, VID_t vid, uint32_t *value)
{
   CHIP_PORT_t chipPort;
   uint32_t fpGroupNum, fpGroupPortNum;
   uint32_t *directAccessReg;

   /* convert iport to chipport */
   chipPort = portNum_slotIPort_to_chipPort(IPORT_TO_SLOT_IPORT(iport));
   fpGroupNum  = omPortToFPGroup[chipPort];
   fpGroupPortNum = omPortToFPPORTGROUP[chipPort];

   /* Get the omVlanTagRamReg address. */
   directAccessReg = asicGetRegMemAddr(0, omVlanTagRamReg[fpGroupNum], 0); 
   ASSERT(directAccessReg);

   /* Register Address is (Front Plane group * 0x4000) + VID * 4 */
   directAccessReg += (fpGroupPortNum * 0x1000) + vid;

   asicDirectMemRead32(0, directAccessReg, value); 

} /* asicReadOmVlanTagRamwithIport */

/**PROC+**********************************************************************
 * Name:      asicReadOmVlanTagRam
 *
 * Purpose:   Wrapper function around asicReadOmVlanTagRamWithIport
 *
 * Params:    pport: physical port 
 *            vid:   VLAN ID
 *            value: address of a variable where the read value will be stored
 *
 * Returns:   none
 *
 **PROC-**********************************************************************/
void asicReadOmVlanTagRam(PPORT_t pport, VID_t vid, uint32_t *value)
{
   asicReadOmVlanTagRamWithIport(PPORT_TO_IPORT(pport), vid, value);
}



code/addrmgr.ss/HAL/BTTF/L2VLans/Slave/bttfHwVlans.c
/**PROC+**********************************************************************
 * Name:      hwUpdateVidRemapTable
 *
 * Purpose:   Update the HW OM VLAN RAM table with remapped vid value. 
 *            This function will get called by bttfHwUpdateVlan when a tagged 
 *            port added to a secondary VLAN, or when a port is removed from 
 *            a VLAN.
 *
 * Params:    pport        - physical port number
 *            vid          - private secondary VLAN ID
 *            assocPrimaryVid      -   private primary VLAN ID
 *            taggedPport     - current tagged pports list of this VLAN
 *            oldMemberPports - Old local member pports of the secondary VLAN
 *            newMemberPports  -New local member pports of the secondary VLAN
 *            oldMappedPports - Old pports mapped to the secondary VLAN
 *            newMappedPports - New pports mapped to the secondary VLAN
 *
 * Returns:   none
 *
 * Note:      
 **PROC-**********************************************************************/
#ifdef FTR_PRIVATE_VLAN
void hwUpdateVidRemapTable(PPORT_t pport, VID_t vid, VID_t assocPrimaryVid,
                           PORT_MAP *taggedPports,
                           PORT_MAP *oldMemberPports, 
                           PORT_MAP *newMemberPports,
                           PORT_MAP *oldMappedPports, 
                           PORT_MAP *newMappedPports)
{
   VID_t remappedVid = vid;
   uint32_t value = 0;
	
   if (is_port_set(taggedPports, pport))
   {
      /* Tagged port added to a secondary VLAN. 
       * Check if its ISL or AccessPort
       */
      if ((is_port_set(newMappedPports, pport)) &&
	       (!is_port_set(oldMemberPports, pport)))
      {
         /* This is new ISL port (PROM->ISL),
          * so remap to the same VID
          */
          if (pvlanMembershipCountPerPromiscuousPort[pport] > 0)
          {
             pvlanMembershipCountPerPromiscuousPort[pport]--;
			    if (pvlanMembershipCountPerPromiscuousPort[pport] == 0)
             {
                asicWriteOmVidRemapSelect(pport, FALSE);
                asicWriteOmVidRemapEnable(pport, FALSE);
             }
          }
       }
       else if ((is_port_set(newMemberPports, pport)) && 
                !(is_port_set(oldMemberPports, pport)))
                     
       {
          pvlanMembershipCountPerAccessTaggedPort[pport]++;
                 
          /* This is Access port. Map it to primary VLAN */
          value = (remappedVid & 0xFFF) | OM_VLAN_CLEAR;
          asicWriteOmVlanTagRam(pport, assocPrimaryVid, value);
         
          if (pvlanMembershipCountPerAccessTaggedPort[pport] == 1)
          {         
             asicWriteOmVidRemapSelect(pport, TRUE);
             asicWriteOmVidRemapEnable(pport, TRUE);
          }   
         
       }
    }
    else
    {
       /* This is not a member port of this VLAN. 
        * Check if its PVLAN mapped port
        */
	    if ((is_port_set(newMappedPports, pport)) &&
	        (is_port_set(oldMappedPports, pport)))
    	 {
          /* check if port type getting changed from 
           * ISL->PROMISCUOUS
           */  
          if ((!is_port_set(newMemberPports, pport)) && 
              (is_port_set(oldMemberPports, pport)))
          {
             remappedVid = assocPrimaryVid;
				 
             value = (remappedVid & 0xFFF) | OM_VLAN_CLEAR;
             asicWriteOmVlanTagRam(pport, vid, value);
						                                           
             pvlanMembershipCountPerPromiscuousPort[pport]++;
							 
             if (pvlanMembershipCountPerPromiscuousPort[pport] == 1)
             {
                asicWriteOmVidRemapSelect(pport, TRUE);
                asicWriteOmVidRemapEnable(pport, TRUE);
             }
          }   
          else
          {
             /* Port already configured as Prom port.
              * No action is required
              */
		       return;
          }   
	    }	 
       else if ((is_port_set(newMappedPports, pport)) &&
                (!is_port_set(oldMappedPports, pport)))
       {
          /* This is a new PVLAN mapped port (promiscuous port), so map it to
           * primary VLAN
           */
          pvlanMembershipCountPerPromiscuousPort[pport]++;
          remappedVid = assocPrimaryVid;

          value = (remappedVid & 0xFFF) | OM_VLAN_CLEAR;
          asicWriteOmVlanTagRam(pport, vid, value);
                           
          if (pvlanMembershipCountPerPromiscuousPort[pport] == 1)
          { 
             asicWriteOmVidRemapSelect(pport, TRUE);
             asicWriteOmVidRemapEnable(pport, TRUE);
          }   
       }
       else if ((!is_port_set(newMappedPports, pport)) &&
                (is_port_set(oldMappedPports, pport)) &&
                (!is_port_set(oldMemberPports, pport)))
       {
          /* Promiscous port getting removed. Disable VID Remap
           * if the port is not member of any priamry VLAN.
           * Don't update VID remap table with default value,
           * (The reason behind not updatin VID remap table is
           *  to avoid overwriting remapped Vid for other promiscuous
           *  ports on same primary VLAN instance if the ports
           *  belong to same fpGroup on Arenal)
           */
          if (pvlanMembershipCountPerPromiscuousPort[pport] > 0)
          {
             pvlanMembershipCountPerPromiscuousPort[pport]--;
          }
							 
          if (pvlanMembershipCountPerPromiscuousPort[pport] == 0)
          {
             asicWriteOmVidRemapSelect(pport, FALSE);
             asicWriteOmVidRemapEnable(pport, FALSE);
          }                    
               
       }
       else if ((!is_port_set(newMemberPports, pport)) && 
                (is_port_set(oldMemberPports, pport)) &&
		     		 (!is_port_set(newMappedPports, pport)))
       {
          /* Access Tagged port getting removed from a sec VLAN */
			
          value = (assocPrimaryVid & 0xFFF) | OM_VLAN_CLEAR;
          asicWriteOmVlanTagRam(pport, assocPrimaryVid, value);
			 
          if (pvlanMembershipCountPerAccessTaggedPort[pport] > 0)
          {
             pvlanMembershipCountPerAccessTaggedPort[pport]--;
          }
		    if (pvlanMembershipCountPerAccessTaggedPort[pport] == 0)
          {
             asicWriteOmVidRemapSelect(pport, FALSE);
             asicWriteOmVidRemapEnable(pport, FALSE);
          }                    
       } 
                   					
   }
}

#endif /* FTR_PRIVATE_VLAN */
