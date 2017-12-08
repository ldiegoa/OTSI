static uint32_t ethertype_to_hw_l3_type(uint16_t ether_type)
{
  
  switch(ether_type)
  {
   case IPV4_TYPE:
	   hl3_type = HW_L3_TYPE_IPV4;
	   break;
   case IPV6_TYPE:
	   l3_type = HW_L3_TYPE_IPV6;
	   break;
   case ARP_TYPE:
	   l3_type = HW_L3_TYPE_ARP;
	   break;
   case IPX_TYPE||IPX2_TYPE:
   l3_type = HW_L3_TYPE_IPX;
   break;
   case APPLETALK_TYPE:
	   l3_type = HW_L3_TYPE_APPLETALK;
	   break;
   case SNA_TYPE :
	   l3_type = HW_L3_TYPE_SNA;
	   break;
   case NETBEUI_TYPE:
	   l3_type = HW_L3_TYPE_NETBEUI;
	   break;
   default :
	   return PV_INVALID_PROTOCOL;
  }
return l3_type;
}

void port_to_ppvid_register_address(uint32_t port_id, uint32_t ppvid_reg_number, uint32_t &ppvid_reg, uint32_t &ppvid_reg_vid, uint32_t &ppvid_reg_l3_type){
  switch (ppvid_reg_number) {

  case 1:
    if (asicTypeofSlotPort(port_number)       == BTTF_GIG_PORT) {
      ppvid_reg           = NEO_MAC_1G_PPVID1_REG;
      ppvid_reg_vid = PPVID1_1G;
      ppvid_reg_l3_type   = L3_TYPE1_1G;
    } else if (asicTypeofSlotPort(port_number) == BTTF_XG_PORT) {
      ppvid_reg           = NEO_MAC_10G_PPVID1_REG;
      ppvid_reg_vid = PPVID1_10G;
      ppvid_reg_l3_type   = L3_TYPE1;
    } else if (asicTypeofSlotPort(port_number) == BTTF_40GIG_PORT) {
      ppvid_reg           = NEO_MAC_40G_PPVID1_REG;
      ppvid_reg_vid = PPVID1_40G;
      ppvid_reg_l3_type   = L3_TYPE1_40G;
    } break;

  case 2:
    if (asicTypeofSlotPort(port_number)       == BTTF_GIG_PORT) {
      ppvid_reg           = NEO_MAC_1G_PPVID2_REG;
      ppvid_reg_vid = PPVID2_1G;
      ppvid_reg_l3_type   = L3_TYPE2_1G;
    } else if (asicTypeofSlotPort(port_number) == BTTF_XG_PORT) {
      ppvid_reg           = NEO_MAC_10G_PPVID2_REG;
      ppvid_reg_vid = PPVID2_10G;
      ppvid_reg_l3_type   = L3_TYPE2;
    } else if (asicTypeofSlotPort(port_number) == BTTF_40GIG_PORT) {
      ppvid_reg           = NEO_MAC_40G_PPVID2_REG;
      ppvid_reg_vid = PPVID2_40G;
      ppvid_reg_l3_type   = L3_TYPE2_40G;
    } break;

  case 3:
    if (asicTypeofSlotPort(port_number)       == BTTF_GIG_PORT) {
      ppvid_reg           = NEO_MAC_1G_PPVID3_REG;
      ppvid_reg_vid = PPVID3_1G;
      ppvid_reg_l3_type   = L3_TYPE3_1G;
    } else if (asicTypeofSlotPort(port_number) == BTTF_XG_PORT) {
      ppvid_reg           = NEO_MAC_10G_PPVID3_REG;
      ppvid_reg_vid = PPVID3_10G;
      ppvid_reg_l3_type   = L3_TYPE3;
    } else if (asicTypeofSlotPort(port_number) == BTTF_40GIG_PORT) {
      ppvid_reg           = NEO_MAC_40G_PPVID3_REG;
      ppvid_reg_vid = PPVID3_40G;
      ppvid_reg_l3_type   = L3_TYPE3_40G;
    }  break;

  case 4:
    if (asicTypeofSlotPort(port_number)       == BTTF_GIG_PORT) {
      ppvid_reg           = NEO_MAC_1G_PPVID4_REG;
      ppvid_reg_vid = PPVID4_1G;
      ppvid_reg_l3_type   = L3_TYPE4_1G;
    } else if (asicTypeofSlotPort(port_number) == BTTF_XG_PORT) {
      ppvid_reg           = NEO_MAC_10G_PPVID4_REG;
      ppvid_reg_vid = PPVID4_10G;
      ppvid_reg_l3_type   = L3_TYPE4;
    } else if (asicTypeofSlotPort(port_number) == BTTF_40GIG_PORT) {
      ppvid_reg           = NEO_MAC_40G_PPVID4_REG;
      ppvid_reg_vid = PPVID4_40G;
      ppvid_reg_l3_type   = L3_TYPE4_40G;
    } break;
  }
}

static int write_ppvid_register(uint32_t  port_number,
                                    uint32_t  ppvid_reg_number,
                                    uint32_t  ethertype,
                                    uint32_t  vid_number) {
  uint32_t macIdx                  = 0;
  uint32_t mac_ppvid_reg           = 0;
  uint32_t mac_ppvid_reg_vid_field = 0;
  uint32_t mac_ppvid_reg_l3_type   = 0;
  uint32_t value                   = 0;
  uint32_t hw_l3_type              = 0;

  hw_l3_type = ethertype_to_hw_l3_type(ether_type);

  /* Assigns of the corresponding Register to be written depending on port
             type and register number */
  port_to_ppvid_register_address(port_number,
                                 ppvid_reg_number,
                                 &mac_ppvid_reg,
                                 &mac_ppvid_reg_vid_field,
                                 &mac_ppvid_reg_l3_type);

  /* Get MAC Index for corresponding port depending on its hw configuration */
  macIdx = asicSlotPortToMACIndex(port_number);

  /* Check vid set or remove (vid = 0) */
  if (0 == vlan_id) {
    /* Set whole register to 0s */
    status = asicWriteReg32(0, mac_ppvid_reg, macIdx, 0);
    if (status) return status;
    return PV_OK;
  }

  status = asicReadReg32(0, mac_ppvid_reg, macIdx, &value);
  if (status) return status;

  asicSetFieldToData(0, mac_ppvid_reg, mac_ppvid_reg_vid_field, vlan_id,
                     &value);
  asicSetFieldToData(0, mac_ppvid_reg, mac_ppvid_reg_l3_type,   hw_l3_type,
                     &value);

  status = asicWriteReg32(0, mac_ppvid_reg, macIdx, value);
  if (status) return status;
  return PV_OK;
}

void read_ppvid_register(uint32_t  port_number,
                                    uint32_t  ppvid_reg_number,
                                    uint32_t *hw_l3_type,
  				    uint32_t *vlan_id)
{
  int status                       = 0;
  uint32_t macIdx                  = 0;
  uint32_t mac_ppvid_reg           = 0;
  uint32_t mac_ppvid_reg_vid_field = 0;
  uint32_t mac_ppvid_reg_l3_type   = 0;
  uint32_t mac_address         = 0;
  uint32_t mac_l3_type             = 0;
  uint32_t vid_value               = 0;

  hw_l3_type = ethertype_to_hw_l3_type(ether_type);

    port_to_ppvid_register_address(port_number,
                                   ppvid_reg_number,
                                   &mac_ppvid_reg,
                                   &mac_ppvid_reg_vid_field,
                                   &mac_ppvid_reg_l3_type);

    /* Get MAC Index for corresponding port depending on its hw configuration */
    macIdx = asicSlotPortToMACIndex(port_number);

    /* Get reg 32 bit address */
    status = asicReadReg32(0, mac_ppvid_reg, macIdx,   &mac_address);
    if (status) return status;

    /* Retrieve VID & ETHER TYPE and com*/
    asicGetFieldFromData(0, mac_ppvid_reg, mac_ppvid_reg_vid_field, &vid_value,
                         mac_address);
    asicGetFieldFromData(0, mac_ppvid_reg, mac_ppvid_reg_l3_type,   &mac_l3_type,
                         mac_address);
      *hw_l3_type 
      *vlan_id = vid_value;
      return PV_OK;

}

