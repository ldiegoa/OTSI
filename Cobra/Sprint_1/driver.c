/*
 * Copyright (C) 2017 Hewlett Packard Enterprise Development LP.
 * All Rights Reserved.
 *
 * The contents of this software are proprietary and confidential to the
 * Hewlett Packard Enterprise Development LP. No part of this program may be
 * photocopied, reproduced, or translated into another programming language
 * without prior written consent of the Hewlett Packard Enterprise
 * Development LP.
 */

// I N C L U D E S
#include <provision/internal/fli/vlans/vlans_subsystem/pvi_vlans_mgr.h>
#include <provision/internal/ucx/pvi_ucx_profile_mgr.h>
#include <provision/internal/core/pvi_driver.h>
#include <provision/pv_profile.h>
#include <provision/internal/utils/matrix/pvi_matrix_util.h>
#include <alloca.h>

#include <legacy/pv/superprimitives/pvi_matrix_hpp_mist.h>
#include <legacy/pv/superprimitives/pvi_matrix_hpp_vlans.h>
#include <legacy/pv/asic/modules/HPP.h>
#include <legacy/pv/asic/modules/OM.h>
#include <legacy/prim.p/primHpp.h>
#include <legacy/pv/asic/asic_specific.h>
#include <legacy/asicPrim.h>
#include <legacy/prim.p/primIpp.h>
#include <legacy/prim.p/primMac.h>
#include <legacy/prim.p/primClue.h>
#include <legacy/prim.p/primFR.h>

#ifdef NEO
# include <legacy/prim.p/neo/omHw.h>
#endif /* ifdef NEO */
#ifdef PACUARE
# include <legacy/prim.p/pacuare/omHw.h>
#endif /* ifdef PACUARE */

#define PVI_LOG_SUBSYSTEM PV_SUBSYSTEM_VLANS

/* Driver name */
#define VLANS_DRIVER_NAME "matrix_vlans_driver"

/* VLANs DB key prefix */
#define DB_KEY_PREFIX "MATRIX_VLANS_"

/* Size of the VLANs Manager DB key prefix*/
#define DB_KEY_PREFIX_SIZE 13

/* Default state for MBV TCAM */
#define HPP_MBV_TCAM_ENABLE 1

// Default MSTP VLAN port state
#define NOT_MEMBER 0

// Pacuare XL 40Gb/s Port number
#define XL_40G_PORT_NUMBER 2

// Pacuare Min X 10 Gb/s Port number
#define X_10G_MIN_PORT_NUMBER 6

// Pacuare Max G 1 Gb/s Port number
#define G_1G_MAX_PORT_NUMBER 57

/* PPVID defines */
#define PPVID_REG_1                     1
#define PPVID_REG_2                     2
#define PPVID_REG_3                     3
#define PPVID_REG_4                     4
#define MAX_PPVID_REGS_PER_PORT         4
#define UNSET_PPVID                     0
#define UNSET_PVID                      0

/* Enum describing the different lookups made to the DB */
enum pvi_vlans_matrix_driver_index_type
{
  /* Driver information */
  PVI_VLANS_DRIVER_MATRIX_INDEX_TYPE_GENERAL_INFO,

  /* Search by the VLAN Number */
  PVI_VLANS_DRIVER_MATRIX_INDEX_TYPE_VLAN_NUMBER,

  /* Search by the port number */
  PVI_VLANS_DRIVER_MATRIX_INDEX_TYPE_PORT,

  /* Search by the port number and the protocol */
  PVI_VLANS_DRIVER_MATRIX_INDEX_TYPE_PORT_AND_PROTOCOL,

  /* Search by the VID list */
  PVI_VLANS_DRIVER_MATRIX_INDEX_TYPE_VID_LIST,

  /* MAX */
  PVI_VLANS_DRIVER_MATRIX_INDEX_TYPE_MAX
};

/* Holds software information about a VLAN in Matrix */
struct pvi_matrix_vlan_data {
  /** Vlan number info */
  struct pvi_vlans_mgr_vlan vlan_mgr;

  /* Bolean to indicate this vlan is valid */
  bool valid;

  /* Bolean to indicate this vlan is illegal */
  bool illegal;

  /** VID associated to this vlan */
  uint32_t vid;
};

/* Key used to get VLAN information from the DB */
struct pvi_vlans_matrix_driver_db_key
{
  /* DB key prefix. It is used to filter VLAN Manager data in the DB */
  char db_key_prefix[DB_KEY_PREFIX_SIZE];

  /* Indicates the type of index in the key (node_id or rep_group_index) */
  enum pvi_vlans_matrix_driver_index_type index_type;

  uint32_t index_1;
  uint32_t index_2;
};

static struct pvi_matrix_vlan_data vlan_data[MAX_MATRIX_NODES][
  MAX_MATRIX_VLAN_NUMBER] = {};

// W R A P P E R  F U N C T I O N S

/* Wrapper to GET values from the DB */
static int pvi_vlans_matrix_driver_db_get_wrapper(
  uint32_t                              node_id,
  struct pvi_vlans_matrix_driver_db_key db_key,
  void                                 *data_ptr,
  size_t                                data_size)
{
  int  status = 0;
  char key[sizeof(struct pvi_vlans_matrix_driver_db_key)];
  struct pvi_ucx_db_data db_data = { 0 };

  memset(&key, 0, sizeof(struct pvi_vlans_matrix_driver_db_key));

  strncpy(db_key.db_key_prefix, DB_KEY_PREFIX, DB_KEY_PREFIX_SIZE);

  // Copy the structure information to the key char pointer
  memcpy(key, &db_key, sizeof(struct pvi_vlans_matrix_driver_db_key));

  db_data.data      = (char *)data_ptr;
  db_data.data_size = data_size;

  status = pvi_ucx_db_get(
    node_id,
    key,
    sizeof(struct pvi_vlans_matrix_driver_db_key),
    &db_data);

  if (status) {
    PVI_LOG(PV_ERROR, "Error getting the data from the DB");
    return status;
  }

  return 0;
}

/* Wrapper to PUT values to the DB */
static int pvi_vlans_matrix_driver_db_put_wrapper(
  uint32_t                              node_id,
  struct pvi_vlans_matrix_driver_db_key db_key,
  void                                 *data_ptr,
  size_t                                data_size)
{
  int  status = 0;
  char key[sizeof(struct pvi_vlans_matrix_driver_db_key)];
  struct pvi_ucx_db_data db_data = { 0 };

  memset(&key, 0, sizeof(struct pvi_vlans_matrix_driver_db_key));

  strncpy(db_key.db_key_prefix, DB_KEY_PREFIX, DB_KEY_PREFIX_SIZE);

  // Copy the structure information to the key char pointer
  memcpy(key, &db_key, sizeof(struct pvi_vlans_matrix_driver_db_key));

  db_data.data      = (char *)data_ptr;
  db_data.data_size = data_size;

  status = pvi_ucx_db_put(
    node_id,
    key,
    sizeof(struct pvi_vlans_matrix_driver_db_key),
    &db_data);

  if (status) {
    PVI_LOG(PV_ERROR, "Error executing the put operation data in the DB");
    return status;
  }

  return 0;
}

/* Wrapper to DELETE values from the DB */
static int pvi_vlans_matrix_driver_db_delete_wrapper(
  uint32_t                              node_id,
  struct pvi_vlans_matrix_driver_db_key db_key)
{
  int  status = 0;
  char key[sizeof(struct pvi_vlans_matrix_driver_db_key)];

  memset(&key, 0, sizeof(struct pvi_vlans_matrix_driver_db_key));

  strncpy(db_key.db_key_prefix, DB_KEY_PREFIX, DB_KEY_PREFIX_SIZE);

  // Copy the structure information to the key char pointer
  memcpy(key, &db_key, sizeof(struct pvi_vlans_matrix_driver_db_key));

  status = pvi_ucx_db_delete(
    node_id,
    key,
    sizeof(struct pvi_vlans_matrix_driver_db_key));

  // Check that no error happened
  if (status) {
    PVI_LOG(PV_ERROR, "Error deleting the data from the DB");
    return status;
  }

  return 0;
}

/* HPP helper functions */
static int pvi_matrix_port_config_initialize(
  uint32_t node_id)
{
  int status = 0;
  uint32_t i = 0;
  uint32_t read_value;

  // read the table starting at 0xF1003008
  // then set to zero the following fields
  // ip_flow_feature_port_and_vlan_mode
  // ip_sa_feature_port_and_vlan_mode
  // ip_flow_sec_port_and_vlan_mode
  // ip_sa_sec_port_and_vlan_mode
  // of the register 2nd register
  for (i = 0; i < MAX_MATRIX_PORTS_PER_NODE; i++) {
    status = pv_registerRead(REGID_NEO_HPP_PORT_OFFSET_2,
                             0,
                             i,
                             &read_value);
    if (status) return status;

    FIELD_SET(&read_value,
              FIELDID_HPP_PORT_OFFSET_2_IP_FLOW_FEATURE_PORT_AND_VLAN_MODE,
              0);
    FIELD_SET(&read_value,
              FIELDID_HPP_PORT_OFFSET_2_IP_SA_FEATURE_PORT_AND_VLAN_MODE,
              0);
    FIELD_SET(&read_value,
              FIELDID_HPP_PORT_OFFSET_2_IP_FLOW_SEC_PORT_AND_VLAN_MODE,
              0);
    FIELD_SET(&read_value,
              FIELDID_HPP_PORT_OFFSET_2_IP_SA_SEC_PORT_AND_VLAN_MODE,
              0);

    status = pv_registerWrite(REGID_NEO_HPP_PORT_OFFSET_2,
                              0,
                              i,
                              read_value);
    if (status) return status;
  }

  return status;
}

static int pvi_matrix_port_config_uninitialize(
  uint32_t node_id)
{
  return pvi_matrix_port_config_initialize(node_id);
}

static int pvi_matrix_mbv_initialize(
  uint32_t node_id)
{
  int status                               = 0;
  uint32_t hpp_mbv_tcam_en_mbv_tcam_enable = 0;

  status =
    pvi_ucx_profile_mgr_int_param_get(node_id,
                                      PVI_PROFILE_CONFIG_TYPE_HW_CONF,
                                      "HPP_MBV_TCAM_ENABLE",
                                      &hpp_mbv_tcam_en_mbv_tcam_enable);
  if (status) return status;

  /* Enable MBV table */
  status = pvi_matrix_hpp_mbv_vrf_tcam_enable(HPP_MBV,
                                              hpp_mbv_tcam_en_mbv_tcam_enable);

  /* Clear all the MBV table entries and enable MBV table */
  asicInitMBVTable();

  return status;
}

static int pvi_matrix_mbv_uninitialize(
  uint32_t node_id)
{
  return pvi_matrix_hpp_mbv_vrf_tcam_enable(HPP_MBV, 0);
}

static int pvi_matrix_remote_vlan_mirroring_initialize(
  uint32_t node_id)
{
  int status                 = 0;
  const uint32_t reset_value = 0;

  /* Set Remote VLAN mirroring registers to 0 */
  status = pv_registerWrite(REGID_R2_HPP_GFR_REMOTE_MIRRORED_VLAN0,
                            0,
                            0,
                            reset_value);
  if (status) return status;

  status = pv_registerWrite(REGID_R2_HPP_GFR_REMOTE_MIRRORED_VLAN1,
                            0,
                            0,
                            reset_value);
  if (status) return status;

  status = pv_registerWrite(REGID_R2_HPP_GFR_REMOTE_MIRRORED_VLAN2,
                            0,
                            0,
                            reset_value);
  if (status) return status;

  status = pv_registerWrite(REGID_R2_HPP_GFR_REMOTE_MIRRORED_VLAN3,
                            0,
                            0,
                            reset_value);

  return status;
}

static int pvi_matrix_remote_vlan_mirroring_uninitialize(
  uint32_t node_id)
{
  return pvi_matrix_remote_vlan_mirroring_initialize(node_id);
}

static int pvi_matrix_vid_to_vlan_initialize(
  uint32_t node_id)
{
  int status = 0;

  asicInitVidToVlanTable();

  return status;
}

static int pvi_matrix_vid_to_vlan_uninitialize(
  uint32_t node_id)
{
  return pvi_matrix_vid_to_vlan_initialize(node_id);
}

static int pvi_matrix_mist_initialize(
  uint32_t node_id)
{
  int status = 0;

  asicInitMistTable();

  return status;
}

static int pvi_matrix_mist_uninitialize(
  uint32_t node_id)
{
  return pvi_matrix_mist_initialize(node_id);
}

/* OM helper functions */
static int pvi_matrix_vid_remap_initialize(
  uint32_t node_id)
{
  int status = 0;

  /* Initialize VLAN tag RAM and COS remap RAM */
  asicInitOmVlanTagRam();

  return status;
}

static int pvi_matrix_vid_remap_uninitialize(
  uint32_t node_id)
{
  return pvi_matrix_vid_remap_initialize(node_id);
}

/* VID management helper functions */

/*
 * Given a ethertype this function returns a CIPP
 * recognizable hw l3
 *
 * param[in]  ethertype (0x0000 - 0xFFFF)
 * param[out]  recognizable hw l3 type(0-15)
 *
 */
static uint32_t pvi_ethertype_to_hw_l3_type(uint16_t ether_type, uint32_t *l3_type)
{
  switch (ether_type)
  {
  case IPV4_TYPE:
    *l3_type = L3TYPE_IPV4;
    break;

  case IPV6_TYPE:
    *l3_type = L3TYPE_IPV6;
    break;

  case ARP_TYPE:
    *l3_type = L3TYPE_ARP;
    break;

  case IPX_TYPE:
    *l3_type = L3TYPE_IPX;
    break;

  case IPX2_TYPE:
    *l3_type = L3TYPE_IPX;
    break;

  case APPLETALK_TYPE:
    *l3_type = L3TYPE_APPLETALK;
    break;

  case SNA_TYPE:
    *l3_type = L3TYPE_SNA;
    break;

  case NETBEUI_TYPE:
    *l3_type = L3TYPE_NETBEUI;
    break;

  default:
    return PV_INVALID_PROTOCOL;
  }
  return PV_OK;
}

/*
 *     This function returns the corresponding register and field
 *     address depending on port type, number and PPVID number(1-4)
 *
 *     param[in] port_number a valid physical port number
 *     param[in] ppvid_reg_number PPVID number(1-4)
 *
 *     param[out] ppvid_reg address of MAC register depending on port type
 *     param[out] ppvid_reg_vid offset of MAC register VID field
 *     param[out] ppvid_reg_l3_type offset of MAC register L3_type field
 *
 *     Each of these 4 ppvid tables is indexed by MAC port
 *     and has the following layout:
 *
 *     31                      16 15      13 12               0
 *     --------------------------------------------------------
 *     |      Unused           |E | l3 type |    VID          |
 *     --------------------------------------------------------
 *     where E = Enable
 */
static void pvi_port_to_ppvid_register_address(uint32_t  port_number,
                                           uint32_t  ppvid_reg_number,
                                           uint32_t *ppvid_reg,
                                           uint32_t *ppvid_reg_vid,
                                           uint32_t *ppvid_reg_l3_type) {
  uint32_t type_of_port = asicTypeofSlotPort(port_number);

  switch (ppvid_reg_number) {
  case PPVID_REG_1:
    if (type_of_port      == BTTF_GIG_PORT) {
      *ppvid_reg         = NEO_MAC_1G_PPVID1_REG;
      *ppvid_reg_vid     = PPVID1_1G;
      *ppvid_reg_l3_type = L3_TYPE1_1G;
    } else if (type_of_port == BTTF_XG_PORT) {
      *ppvid_reg         = NEO_MAC_10G_PPVID1_REG;
      *ppvid_reg_vid     = PPVID1_10G;
      *ppvid_reg_l3_type = L3_TYPE1;
    } else if (type_of_port == BTTF_40GIG_PORT) {
      *ppvid_reg         = NEO_MAC_40G_PPVID1_REG;
      *ppvid_reg_vid     = PPVID1_40G;
      *ppvid_reg_l3_type = L3_TYPE1_40G;
    } break;

  case PPVID_REG_2:
    if (type_of_port       == BTTF_GIG_PORT) {
      *ppvid_reg         = NEO_MAC_1G_PPVID2_REG;
      *ppvid_reg_vid     = PPVID2_1G;
      *ppvid_reg_l3_type = L3_TYPE2_1G;
    } else if (type_of_port == BTTF_XG_PORT) {
      *ppvid_reg         = NEO_MAC_10G_PPVID2_REG;
      *ppvid_reg_vid     = PPVID2_10G;
      *ppvid_reg_l3_type = L3_TYPE2;
    } else if (type_of_port == BTTF_40GIG_PORT) {
      *ppvid_reg         = NEO_MAC_40G_PPVID2_REG;
      *ppvid_reg_vid     = PPVID2_40G;
      *ppvid_reg_l3_type = L3_TYPE2_40G;
    } break;

  case PPVID_REG_3:
    if (type_of_port       == BTTF_GIG_PORT) {
      *ppvid_reg         = NEO_MAC_1G_PPVID3_REG;
      *ppvid_reg_vid     = PPVID3_1G;
      *ppvid_reg_l3_type = L3_TYPE3_1G;
    } else if (type_of_port == BTTF_XG_PORT) {
      *ppvid_reg         = NEO_MAC_10G_PPVID3_REG;
      *ppvid_reg_vid     = PPVID3_10G;
      *ppvid_reg_l3_type = L3_TYPE3;
    } else if (type_of_port == BTTF_40GIG_PORT) {
      *ppvid_reg         = NEO_MAC_40G_PPVID3_REG;
      *ppvid_reg_vid     = PPVID3_40G;
      *ppvid_reg_l3_type = L3_TYPE3_40G;
    }  break;

  case PPVID_REG_4:
    if (type_of_port       == BTTF_GIG_PORT) {
      *ppvid_reg         = NEO_MAC_1G_PPVID4_REG;
      *ppvid_reg_vid     = PPVID4_1G;
      *ppvid_reg_l3_type = L3_TYPE4_1G;
    } else if (type_of_port == BTTF_XG_PORT) {
      *ppvid_reg         = NEO_MAC_10G_PPVID4_REG;
      *ppvid_reg_vid     = PPVID4_10G;
      *ppvid_reg_l3_type = L3_TYPE4;
    } else if (type_of_port == BTTF_40GIG_PORT) {
      *ppvid_reg         = NEO_MAC_40G_PPVID4_REG;
      *ppvid_reg_vid     = PPVID4_40G;
      *ppvid_reg_l3_type = L3_TYPE4_40G;
    } break;
  }
}

/*
 * Given a port number and PPVID register number(1-4)
 * this function writes its VID and l3 type fields
 *
 *   param[in] port_number a valid physical port number
 *   param[in] ppvid_reg_number PPVID number(1-4)
 *   param[in] hw_l3_type a l3 type from 0 to 15
 *   param[in] vid_number a valid VID number from 1 to 4095
 *
 *   For Pacuare, this feature is supported for port 2(XL),
 *   ports 6 to 9(XG), and ports from 10 to 57(GIG ports)
 */
static int pvi_ppvid_register_write(uint32_t port_number,
                                uint32_t ppvid_reg_number,
                                uint32_t hw_l3_type,
                                uint32_t vid_number) {
  int status                       = 0;
  uint32_t macIdx                  = 0;
  uint32_t mac_ppvid_reg           = 0;
  uint32_t mac_ppvid_reg_vid_field = 0;
  uint32_t mac_ppvid_reg_l3_type   = 0;
  uint32_t mac_reg_address         = 0;

#ifdef PACUARE
  // Recirculation ports not supported by this feature
  if ((X_10G_MIN_PORT_NUMBER > port_number) &&
      (XL_40G_PORT_NUMBER != port_number)) return PV_INVALID_PORT;

  // DMA ports and Conduit ports not supported by this feature
  if (G_1G_MAX_PORT_NUMBER < port_number) return PV_INVALID_PORT;
#endif

#ifdef NEO
  if(1 >= port_number || 33 < port_number) return PV_INVALID_PORT;
#endif

  if (PV_MAX_VID_NUMBER < vid_number) return PV_OUT_OF_RANGE;

#ifdef PACUARE
  // Check if the port is on reset
  uint32_t macMipMiscConfigRegData = 0;
  uint32_t IsInReset               = 0;
  asicMipmiscReadReg32(0, NG_MIPMISC_MAC_CONFIG, port_number,
                       &macMipMiscConfigRegData);
  asicGetFieldFromData(0, NG_MIPMISC_MAC_CONFIG, MAC_MAC_RESET_L,
                       &IsInReset, macMipMiscConfigRegData);

  if (0 == IsInReset) return PV_BUSY;
#endif

  /*
   * Assigning of the corresponding Register to
   * be written depending on port type and register number
   */
  pvi_port_to_ppvid_register_address(port_number,
                                 ppvid_reg_number,
                                 &mac_ppvid_reg,
                                 &mac_ppvid_reg_vid_field,
                                 &mac_ppvid_reg_l3_type);

  /* Get MAC Index for corresponding port depending on its hw configuration */
  macIdx = asicSlotPortToMACIndex(port_number);

  /* Check vid set or remove (vid = 0) */
  if (UNSET_PPVID == vid_number) {
    /* Set whole register to 0s */
    status = asicWriteReg32(0, mac_ppvid_reg, macIdx, 0);
    return status;
  }

  status = asicReadReg32(0, mac_ppvid_reg, macIdx, &mac_reg_address);

  if (status) return status;

  asicSetFieldToData(0, mac_ppvid_reg, mac_ppvid_reg_vid_field, vid_number,
                     &mac_reg_address);
  asicSetFieldToData(0, mac_ppvid_reg, mac_ppvid_reg_l3_type,   hw_l3_type,
                     &mac_reg_address);

  status = asicWriteReg32(0, mac_ppvid_reg, macIdx, mac_reg_address);

  return status;
}

/*
 * Given a port number and PPVID register number(1-4)
 * this function retrieves its VID and l3 type fields data
 *
 *   param[in] port_number a valid physical port number
 *   param[in] ppvid_reg_number PPVID number(1-4)
 *
 *   param[out] hw_l3_type returns a l3 type from 0 to 15
 *   param[out] vid_number returns a valid VID number from 1 to 4095
 *
 *   For Pacuare, this feature is supported for port 2(XL),
 *   ports 6 to 9(XG), and ports from 10 to 57(GIG ports)
 */
static int pvi_pvid_register_read(uint32_t  port_number,
                               uint32_t  ppvid_reg_number,
                               uint32_t *hw_l3_type,
                               uint32_t *vlan_id)
{
  int status                       = 0;
  uint32_t macIdx                  = 0;
  uint32_t mac_ppvid_reg           = 0;
  uint32_t mac_ppvid_reg_vid_field = 0;
  uint32_t mac_ppvid_reg_l3_type   = 0;
  uint32_t mac_reg_address         = 0;
  uint32_t mac_l3_type             = 0;
  uint32_t vid_value               = 0;

#ifdef PACUARE
  // Recirculation ports not supported by this feature
  if ((X_10G_MIN_PORT_NUMBER > port_number) &&
      (XL_40G_PORT_NUMBER != port_number)) return PV_INVALID_PORT;

  // DMA ports and Conduit ports not supported by this feature
  if (G_1G_MAX_PORT_NUMBER < port_number) return PV_INVALID_PORT;
#endif

#ifdef NEO
  if(1 >= port_number || 33 < port_number) return PV_INVALID_PORT;
#endif

#ifdef PACUARE
  // Check if the port is on reset
  uint32_t macMipMiscConfigRegData = 0;
  uint32_t IsInReset               = 0;
  asicMipmiscReadReg32(0, NG_MIPMISC_MAC_CONFIG, port_number,
                       &macMipMiscConfigRegData);
  asicGetFieldFromData(0, NG_MIPMISC_MAC_CONFIG, MAC_MAC_RESET_L,
                       &IsInReset, macMipMiscConfigRegData);

  if (0 == IsInReset) return PV_BUSY;
#endif

  pvi_port_to_ppvid_register_address(port_number,
                                 ppvid_reg_number,
                                 &mac_ppvid_reg,
                                 &mac_ppvid_reg_vid_field,
                                 &mac_ppvid_reg_l3_type);

  /* Get MAC Index for corresponding port depending on its hw configuration */
  macIdx = asicSlotPortToMACIndex(port_number);

  /* Get reg 32 bit address */
  status = asicReadReg32(0, mac_ppvid_reg, macIdx,   &mac_reg_address);
  if (status) return status;

  /* Retrieve VID & ETHER TYPE */
  asicGetFieldFromData(0, mac_ppvid_reg, mac_ppvid_reg_vid_field, &vid_value,
                       mac_reg_address);
  asicGetFieldFromData(0, mac_ppvid_reg, mac_ppvid_reg_l3_type,   &mac_l3_type,
                       mac_reg_address);
  *hw_l3_type = mac_l3_type;
  *vlan_id    = vid_value;
  return PV_OK;
}

// V L A N    D R I V E R   F U N C T I O N S

/* The concept of creating a VLAN instance in Matrix is merely a SW concept */
static int pvi_vlans_driver_vlan_create(
  uint32_t node_id,
  uint32_t vlan_number)
{
  int status = 0;

  if (0 == vlan_number) return PV_INVALID_VLAN;

  vlan_data[node_id][vlan_number].vlan_mgr.vlan_number = vlan_number;
  vlan_data[node_id][vlan_number].valid                = true;
  vlan_data[node_id][vlan_number].illegal              = true;

  status = pv_vports_bitmap_init(
    0, node_id, &vlan_data[node_id][vlan_number].vlan_mgr.vlan_vports);

  return status;
}

static int pvi_vlans_driver_vlan_remove(
  uint32_t node_id,
  uint32_t vlan_number)
{
  if (0 == vlan_number) return PV_INVALID_VLAN;

  /* Check if there are any ports attached */
  if ((0 != vlan_data[node_id][vlan_number].vlan_mgr.vlan_vports.value[0]) ||
      (0 != vlan_data[node_id][vlan_number].vlan_mgr.vlan_vports.value[1])) {
    return PV_BUSY;
  }

  /* Check if there are any nodes attached */
  if (vlan_data[node_id][vlan_number].vlan_mgr.nodes[0]) return PV_BUSY;

  vlan_data[node_id][vlan_number].vlan_mgr.vlan_number = 0;
  vlan_data[node_id][vlan_number].valid                = false;

  return 0;
}

static int pvi_vlans_driver_illegal_vlan_set(
  uint32_t node_id,
  uint32_t vlan_number,
  bool     illegal)
{
  int status   = 0;
  uint32_t vid = vlan_data[node_id][vlan_number].vid;

  struct pvi_matrix_hpp_vid_to_vlan_entry vid_to_vlan_entry = {};

  // Check if Vlan instance exists
  if (vlan_data[node_id][vlan_number].valid == false) return PV_PARM;

  // Default vlan should not be used
  if (vid == 0) return PV_PARM;

  // Read Vid2Vlan Table
  status = pvi_matrix_hpp_vid_to_vlan_read(vid, &vid_to_vlan_entry);
  if (status) return status;

  vid_to_vlan_entry.word1.bits.src_vlan_num_illegal = illegal ? 1 : 0;

  // Write to Vid2Vlan Table
  status = pvi_matrix_hpp_vid_to_vlan_write(vid, &vid_to_vlan_entry);
  if (status) return status;

  vlan_data[node_id][vlan_number].illegal = true;

  return status;
}

static int pvi_vlans_driver_illegal_vlan_get(
  uint32_t node_id,
  uint32_t vlan_number,
  bool    *illegal)
{
  int status   = 0;
  uint32_t vid = vlan_data[node_id][vlan_number].vid;

  struct pvi_matrix_hpp_vid_to_vlan_entry vid_to_vlan_entry = {};

  // Check if Vlan instance exists
  if (vlan_data[node_id][vlan_number].valid == false) return PV_PARM;

  // Default vlan should not be used
  if (vid == 0) return PV_PARM;

  // Read Vid2Vlan Table
  status = pvi_matrix_hpp_vid_to_vlan_read(vid, &vid_to_vlan_entry);
  if (status) return status;

  *illegal = vid_to_vlan_entry.word1.bits.src_vlan_num_illegal;

  // This shouldn't happen
  if (*illegal !=
      vlan_data[node_id][vlan_number].illegal) return PV_INVALID_CONFIG;

  return status;
}

/* To map  VID to a VLAN in Matrix we must:
    - Update the VID2VLAN Table to map the VID to the VLAN number VID2VLAN
    - Set illegal_src_vlan_number = false */
static int pvi_vlans_driver_vid_to_vlan_set(
  uint32_t node_id,
  uint32_t vid,
  uint32_t vlan_number)
{
  int status = 0;

  if (vlan_data[node_id][vlan_number].valid == false) return PV_PARM;

  if (PV_MAX_VID_NUMBER < vid) return PV_OUT_OF_RANGE;

  if ((PV_MAX_VID_NUMBER == vid) || (0 == vid)) return PV_INVALID_VLAN;

  struct pvi_matrix_hpp_vid_to_vlan_entry vid_to_vlan_entry = {};

  vlan_data[node_id][vlan_number].vid = vid;

  // Read Vid2Vlan Table
  status = pvi_matrix_hpp_vid_to_vlan_read(vid, &vid_to_vlan_entry);
  if (status) return status;

  vid_to_vlan_entry.word1.bits.vlan_number = vlan_number;

  // FIXME: should we update the illegal field too?
  vid_to_vlan_entry.word1.bits.src_vlan_num_illegal = 0;

  // Write to Vid2Vlan Table
  status = pvi_matrix_hpp_vid_to_vlan_write(vid, &vid_to_vlan_entry);
  if (status) return status;

  return status;
}

static int pvi_vlans_driver_vid_to_vlan_get(
  uint32_t  node_id,
  uint32_t  vid,
  uint32_t *vlan_number)
{
  int status = 0;

  struct pvi_matrix_hpp_vid_to_vlan_entry vid_to_vlan_entry = {};

  if (PV_MAX_VID_NUMBER < vid) return PV_OUT_OF_RANGE;

  if ((PV_MAX_VID_NUMBER == vid) || (0 == vid)) return PV_INVALID_VLAN;

  // Read Vid2Vlan Table
  status = pvi_matrix_hpp_vid_to_vlan_read(vid, &vid_to_vlan_entry);
  if (status) return status;

  *vlan_number = vid_to_vlan_entry.word1.bits.vlan_number;

  // This shouldn't happen
  if (*vlan_number !=
      vlan_data[node_id][*vlan_number].vlan_mgr.vlan_number) return
      PV_INVALID_CONFIG;

  return status;
}

/* To map  VLAN to a VID in Matrix we must:
    - Update the OM VLAN to VID remap Table  */
static int pvi_vlans_driver_vlan_to_vid_set(
  uint32_t node_id,
  uint32_t vlan_number,
  uint32_t vid)
{
  int status = PV_FEATURE_UNAVAILABLE;

  // TODO: Update the OM_VLAN_REMAP_TABLE

  return status;
}

static int pvi_vlans_driver_vlan_to_vid_get(
  uint32_t  node_id,
  uint32_t  vlan_number,
  uint32_t *vid)
{
  int status = PV_FEATURE_UNAVAILABLE;

  // TODO: Read from OM_VLAN_REMAP_TABLE
  return status;
}

static int pvi_vlans_driver_port_vid_set(
  uint32_t node_id,
  uint32_t vlan_id,
  uint32_t port_number)
{
  int status                       = 0;
  uint32_t macIdx                  = 0;
  uint32_t mac_pvid_reg            = 0;
  uint32_t mac_pvid_reg_vid_field  = 0;
  uint32_t value                   = 0;
  uint32_t type_of_port            = 0;

#ifdef PACUARE

  // Recirculation ports not supported by this feature
  if ((X_10G_MIN_PORT_NUMBER > port_number) &&
      (XL_40G_PORT_NUMBER != port_number)) return PV_INVALID_PORT;

  // DMA ports and Conduit ports not supported by this feature
  if (G_1G_MAX_PORT_NUMBER < port_number) return PV_INVALID_PORT;

  if (PV_MAX_VID_NUMBER < vlan_id) return PV_OUT_OF_RANGE;
#endif

#ifdef NEO
  if(1 >= port_number || 33 < port_number) return PV_INVALID_PORT;
#endif

#ifdef PACUARE
  // Check if the port is on reset
  uint32_t macMipMiscConfigRegData = 0;
  uint32_t IsInReset               = 0;
  asicMipmiscReadReg32(0, NG_MIPMISC_MAC_CONFIG, port_number,
                       &macMipMiscConfigRegData);
  asicGetFieldFromData(0, NG_MIPMISC_MAC_CONFIG, MAC_MAC_RESET_L,
                       &IsInReset, macMipMiscConfigRegData);

  if (0 == IsInReset) return PV_BUSY;
#endif

  type_of_port = asicTypeofSlotPort(port_number);

  /*
   * Selection of the corresponding Register to be written
   * depending on port type
   */
  if (type_of_port      == BTTF_GIG_PORT) {
    mac_pvid_reg           = NEO_MAC_1G_PVID_REG;
    mac_pvid_reg_vid_field = PVID;
  } else if (type_of_port == BTTF_XG_PORT) {
    mac_pvid_reg           = NEO_MAC_10G_PVID_REG;
    mac_pvid_reg_vid_field = PVID_10G;
  } else if (type_of_port == BTTF_40GIG_PORT) {
    mac_pvid_reg           = NEO_MAC_40G_PVID_REG;
    mac_pvid_reg_vid_field = PVID_40G;
  }

  /* Get MAC Index for corresponding port depending on its hw configuration */
  macIdx = asicSlotPortToMACIndex(port_number);

  /* Check vid set or remove (vid = 0) */
  if (UNSET_PVID == vlan_id) {
    /* Set whole register to 0s */
    status = asicWriteReg32(0, mac_pvid_reg, macIdx, 0);
    return status;
  }

  /* If vid is different to 0, proceed to PVID field writing */
  status = asicReadReg32(0, mac_pvid_reg, macIdx, &value);
  if (status) return status;

  asicSetFieldToData(0, mac_pvid_reg, mac_pvid_reg_vid_field, vlan_id, &value);
  status = asicWriteReg32(0, mac_pvid_reg, macIdx, value);
  return status;
}

static int pvi_vlans_driver_protocol_vid_set(
  uint32_t node_id,
  uint32_t vlan_id,
  uint32_t port_number,
  uint16_t ether_type)
{
  int status           = 0;
  uint32_t hw_l3_type  = 0;
  uint32_t vid_value   = 0;
  uint32_t mac_l3_type = 0;

#ifdef PACUARE
  // Recirculation ports not supported by this feature
  if ((X_10G_MIN_PORT_NUMBER > port_number) &&
      (XL_40G_PORT_NUMBER != port_number)) return PV_INVALID_PORT;

  // DMA ports and Conduit ports not supported by this feature
  if (G_1G_MAX_PORT_NUMBER < port_number) return PV_INVALID_PORT;
#endif

#ifdef NEO
  if(1 >= port_number || 33 < port_number) return PV_INVALID_PORT;
#endif

  if (PV_MAX_VID_NUMBER < vlan_id) return PV_OUT_OF_RANGE;

  status = pvi_ethertype_to_hw_l3_type(ether_type, &hw_l3_type);
  if (status) return status;

  /*
   *   PPVID registers:
   *        (1) IPv4 must be in the 3rd PPVID register if you want it
   *            to also include IPv4 error type.
   *        (2) IPv6 must be in the 2nd PPVID register if you want it
   *            to also include IPv6 error type and IPv6 other type.
   *        (3) If IPV4 type is put in the 4th PPVID register, it will
   *            also include IPv4 error type and ARP type.
   *        (4) The 1st PPVID register has no unique characteristics.
   */
  if (L3TYPE_IPV4 == hw_l3_type) {
    status = pvi_ppvid_register_write(port_number,
                                  PPVID_REG_3,
                                  hw_l3_type,
                                  vlan_id);
    return status;
  } else if (L3TYPE_IPV6 == hw_l3_type) {
    status = pvi_ppvid_register_write(port_number,
                                  PPVID_REG_2,
                                  hw_l3_type,
                                  vlan_id);
    return status;
  } else {
    /*
     * just find a empty one, giving preference to the 1st and 4th
     * in case IPv4 or IPV6 is defined later.
     */
    status = pvi_pvid_register_read(port_number,
                                 PPVID_REG_1,
                                 &mac_l3_type,
                                 &vid_value);
    if (status) return status;

    if (UNSET_PPVID   == vid_value) {
      status = pvi_ppvid_register_write(port_number,
                                    PPVID_REG_1,
                                    hw_l3_type,
                                    vlan_id);
      return status;
    }
    status = pvi_pvid_register_read(port_number,
                                 PPVID_REG_4,
                                 &mac_l3_type,
                                 &vid_value);
    if (status) return status;

    if (UNSET_PPVID  == vid_value) {
      status = pvi_ppvid_register_write(port_number,
                                    PPVID_REG_4,
                                    hw_l3_type,
                                    vlan_id);
      return status;
    }
    status = pvi_pvid_register_read(port_number,
                                 PPVID_REG_3,
                                 &mac_l3_type,
                                 &vid_value);
    if (status) return status;

    if (UNSET_PPVID   == vid_value) {
      status = pvi_ppvid_register_write(port_number,
                                    PPVID_REG_3,
                                    hw_l3_type,
                                    vlan_id);
      return status;
    }
    status = pvi_pvid_register_read(port_number,
                                 PPVID_REG_2,
                                 &mac_l3_type,
                                 &vid_value);
    if (status) return status;

    if (UNSET_PPVID == vid_value) {
      status = pvi_ppvid_register_write(port_number,
                                    PPVID_REG_2,
                                    hw_l3_type,
                                    vlan_id);
      return status;
    }
  }

  /* In case that no free PPVID was found, VID is assigned to PPVID1 */
  status = pvi_ppvid_register_write(port_number,
                                PPVID_REG_1,
                                hw_l3_type,
                                vlan_id);
  return status;
}

static int pvi_vlans_driver_protocol_vid_get(
  uint32_t  node_id,
  uint32_t  port_number,
  uint16_t  ether_type,
  uint32_t *vlan_id)
{
  int status           = 0;
  uint32_t mac_l3_type = 0;
  uint32_t vid_value   = 0;
  uint32_t hw_l3_type  = 0;
  int ppvid_reg_number = 0;

#ifdef PACUARE
  // Recirculation ports not supported by this feature
  if ((X_10G_MIN_PORT_NUMBER > port_number) &&
      (XL_40G_PORT_NUMBER != port_number)) return PV_INVALID_PORT;

  // DMA ports and Conduit ports not supported by this feature
  if (G_1G_MAX_PORT_NUMBER < port_number) return PV_INVALID_PORT;
#endif

#ifdef NEO
  if(1 >= port_number || 33 < port_number) return PV_INVALID_PORT;
#endif

  status = pvi_ethertype_to_hw_l3_type(ether_type, &hw_l3_type);
  if (status) return status;

  /* Look up in each PPVID register(1-4) to find a vid match */
  for (ppvid_reg_number = PPVID_REG_1;
       ppvid_reg_number <= MAX_PPVID_REGS_PER_PORT;
       ppvid_reg_number++) {
    status = pvi_pvid_register_read(port_number,
                                 ppvid_reg_number,
                                 &mac_l3_type,
                                 &vid_value);
    if (status) return status;

    if (hw_l3_type == mac_l3_type) {
      *vlan_id = vid_value;
      return PV_OK;
    }
  }

  /*
   * If no ethertype matched vid 0 is returned, which means that the protocol
   * has not been set on this port
   */
  *vlan_id = UNSET_PPVID;
  return PV_OK;
}

static int pvi_vlans_driver_hw_initialize(
  uint32_t node_id)
{
  int status = 0;

  /* TODO:
   * - DECLARE Registers for ports configuration
   * - Set default VLAN for untagged pkts
   */

  /* Init. HPP */
  status = pvi_matrix_port_config_initialize(node_id);
  if (status) return status;

  status = pvi_matrix_mbv_initialize(node_id);
  if (status) return status;

  status = pvi_matrix_vid_to_vlan_initialize(node_id);
  if (status) return status;

  status = pvi_matrix_mist_initialize(node_id);
  if (status) return status;

  status = pvi_matrix_remote_vlan_mirroring_initialize(node_id);
  if (status) return status;

  /* Init. OM */
  status = pvi_matrix_vid_remap_initialize(node_id);

  return status;
}

static int pvi_vlans_driver_hw_uninitialize(
  uint32_t node_id)
{
  int status = 0;

  /* Uninit. HPP */
  status = pvi_matrix_port_config_uninitialize(node_id);
  if (status) return status;

  status = pvi_matrix_mbv_uninitialize(node_id);
  if (status) return status;

  status = pvi_matrix_vid_to_vlan_uninitialize(node_id);
  if (status) return status;

  status = pvi_matrix_mist_uninitialize(node_id);
  if (status) return status;

  status = pvi_matrix_remote_vlan_mirroring_uninitialize(node_id);
  if (status) return status;

  /* Uninit. OM */
  status = pvi_matrix_vid_remap_uninitialize(node_id);

  return status;
}

static int pvi_vlans_driver_initialize(
  uint32_t node_id)
{
  int status = 0;
  uint32_t i = 0;

  struct pvi_vlans_mgr_general_info driver_general_info = {};
  struct pvi_vlans_matrix_driver_db_key db_key = {};

  for (i = 0; i < MAX_MATRIX_VLAN_NUMBER; i++) {
    vlan_data[node_id][i].illegal = true;
    vlan_data[node_id][i].vid     = 0;
    vlan_data[node_id][i].valid   = false;
  }

  // Check if the VLAN Driver already existed
  db_key.index_type = PVI_VLANS_DRIVER_MATRIX_INDEX_TYPE_GENERAL_INFO;

  status = pvi_vlans_matrix_driver_db_get_wrapper(
    node_id,
    db_key,
    &driver_general_info,
    sizeof(struct pvi_vlans_mgr_general_info));

  if (-ENODATA != status) return PV_FEATURE_ALREADY_INITIALIZED;

  driver_general_info.vlan_count = 0;

  status = pvi_vlans_matrix_driver_db_put_wrapper(
    node_id,
    db_key,
    &driver_general_info,
    sizeof(struct pvi_vlans_mgr_general_info));
  if (status) return status;

  return pvi_vlans_driver_hw_initialize(node_id);
}

static int pvi_vlans_driver_uninitialize(
  uint32_t node_id)
{
  int status = 0;
  uint32_t i = 0;

  struct pvi_vlans_mgr_general_info driver_general_info = {};
  struct pvi_vlans_matrix_driver_db_key db_key = {};

  for (i = 0; i < MAX_MATRIX_VLAN_NUMBER; i++) {
    if (vlan_data[node_id][i].valid == true) return PV_BUSY;
  }

  // Check if the VLAN Driver already existed
  db_key.index_type = PVI_VLANS_DRIVER_MATRIX_INDEX_TYPE_GENERAL_INFO;

  status = pvi_vlans_matrix_driver_db_get_wrapper(
    node_id,
    db_key,
    &driver_general_info,
    sizeof(struct pvi_vlans_mgr_general_info));
  if (status) return status;

  if (0 != driver_general_info.vlan_count) {
    PVI_LOG(PV_ERROR, "Some VLANs are still active");
    return PV_BUSY;
  }

  status = pvi_vlans_matrix_driver_db_delete_wrapper(node_id, db_key);
  if (status) return status;

  return pvi_vlans_driver_hw_uninitialize(node_id);
}

static int pvi_vlans_driver_mstp_port_state_set(
  uint32_t                      node_id,
  struct pv_vport               vport,
  uint32_t                      vlan_number,
  enum pv_vlans_mstp_port_state state)
{
  int status           = 0;
  uint32_t port_number = 0;
  bool     in_vlan     = false;
  uint32_t mstp_state  = 0;

  if (vlan_data[node_id][vlan_number].valid == false) return PV_PARM;

  status = pv_vports_port_get(vport, &port_number);
  if (status) return status;

  /* Check that the port belongs to the VLAN */
  status = pv_vports_phys_port_in_bitmap_is(
    vport, vlan_data[node_id][vlan_number].vlan_mgr.vlan_vports, &in_vlan);
  if (status) return status;

  if (!in_vlan) return PV_PARM;

  switch (state) {
  case PV_VLAN_MSTP_STATE_LEARNING:
    mstp_state = PVI_MIST_LEARNING;
    break;

  case PV_VLAN_MSTP_STATE_FORWARDING:
    mstp_state = PVI_MIST_FORWARDING;
    break;

  case PV_VLAN_MSTP_STATE_BLOCKED:
    mstp_state = PVI_MIST_BLOCKED;
    break;

  default:
    return PV_PARM;
  }

  status = pvi_matrix_hpp_mist_state_set(
    vlan_number, port_number, mstp_state);
  if (status) return status;

  return status;
}

static int pvi_vlans_driver_mstp_port_state_get(
  uint32_t                       node_id,
  struct pv_vport                vport,
  uint32_t                       vlan_number,
  enum pv_vlans_mstp_port_state *state)
{
  int status           = 0;
  uint32_t port_number = 0;
  bool     in_vlan     = false;
  uint32_t mstp_state  = 0;

  if (vlan_data[node_id][vlan_number].valid == false) return PV_PARM;

  status = pv_vports_port_get(vport, &port_number);
  if (status) return status;

  /* Check that the port belongs to the VLAN */
  status = pv_vports_phys_port_in_bitmap_is(
    vport, vlan_data[node_id][vlan_number].vlan_mgr.vlan_vports, &in_vlan);
  if (status) return status;

  if (!in_vlan) return PV_INVALID_PORT;

  status = pvi_matrix_hpp_mist_state_get(
    vlan_number, port_number, &mstp_state);
  if (status) return status;

  switch (mstp_state) {
  case PVI_MIST_LEARNING:
    *state = PV_VLAN_MSTP_STATE_LEARNING;
    break;

  case PVI_MIST_FORWARDING:
    *state = PV_VLAN_MSTP_STATE_FORWARDING;
    break;

  case PVI_MIST_BLOCKED:
    *state = PV_VLAN_MSTP_STATE_BLOCKED;
    break;

  default:
    return PV_PARM;
  }

  return status;
}

/** This function adds a vport in either physical or bitmap format
 *  to a VLAN.  In Matrix it needs to configure the following tables:
 *    -MIST table
 *    -FR VLAN filter table */
static int pvi_vlans_driver_port_add(
  uint32_t        node_id,
  struct pv_vport vport,
  uint32_t        vlan_number)
{
  int status               = 0;
  uint32_t port_number     = 0;
  bool     in_bitmap       = false;
  uint64_t filter_mask     = 0;
  uint32_t filter_entry[2] = { 0 };

  status = pv_vports_port_get(vport, &port_number);
  if (status) return status;

  /* Check if the vPort is already in the VLAN */
  status = pv_vports_phys_port_in_bitmap_is(
    vport, vlan_data[node_id][vlan_number].vlan_mgr.vlan_vports, &in_bitmap);
  if (status) return status;

  if (in_bitmap) return PV_OK;

  /* Add vport to Vlan data */
  status = pv_vports_bitmap_vport_add(
    vport, &vlan_data[node_id][vlan_number].vlan_mgr.vlan_vports);
  if (status) return status;

  /* Update FR Vlan Filter Table */
  asicFRReadFilterEntry(VLAN_PORT_FILTER, vlan_number, filter_entry);

  memcpy(&filter_mask, filter_entry, sizeof(filter_entry));
  filter_mask    |= (uint64_t)1 << port_number;
  filter_entry[0] = filter_mask;
  filter_entry[1] = filter_mask >> 32;

  asicFRWriteFilterEntry(VLAN_PORT_FILTER, vlan_number, filter_entry);

  /* Update MIST state for this port */
  status = pvi_vlans_driver_mstp_port_state_set(node_id,
                                                vport,
                                                vlan_number,
                                                PV_VLAN_MSTP_STATE_BLOCKED);

  return status;
}

/** This function removes a vport in either physical or bitmap format
 *  from a VLAN.  In Matrix it needs to configure the following tables:
 *    -MIST table
 *    -FR VLAN filter table */
static int pvi_vlans_driver_port_remove(
  uint32_t        node_id,
  struct pv_vport vport,
  uint32_t        vlan_number)
{
  int status               = 0;
  uint32_t port_number     = 0;
  bool     in_bitmap       = false;
  uint64_t filter_mask     = 0;
  uint32_t filter_entry[2] = { 0 };

  status = pv_vports_port_get(vport, &port_number);
  if (status) return status;

  /* Check if the vPort is already in the VLAN */
  status = pv_vports_phys_port_in_bitmap_is(
    vport, vlan_data[node_id][vlan_number].vlan_mgr.vlan_vports, &in_bitmap);
  if (status) return status;

  if (!in_bitmap) {
    asicFRReadFilterEntry(VLAN_PORT_FILTER, vlan_number, filter_entry);
    memcpy(&filter_mask, filter_entry, sizeof(filter_entry));
    filter_mask &= (uint64_t)1 << port_number;
    if (!filter_mask) return PV_NOT_FOUND;
  }

  /* Remove vport of Vlan data */
  status = pv_vports_bitmap_vport_remove(
    vport, &vlan_data[node_id][vlan_number].vlan_mgr.vlan_vports);
  if (status) return status;

  /* Update FR Vlan Filter Table */
  asicFRReadFilterEntry(VLAN_PORT_FILTER, vlan_number, filter_entry);

  memcpy(&filter_mask, filter_entry, sizeof(filter_entry));
  filter_mask    &= ~((uint64_t)1 << port_number);
  filter_entry[0] = filter_mask;
  filter_entry[1] = filter_mask >> 32;

  asicFRWriteFilterEntry(VLAN_PORT_FILTER, vlan_number, filter_entry);

  /* Update MIST state for this port */
  status = pvi_matrix_hpp_mist_state_set(vlan_number, port_number, NOT_MEMBER);
  if (status) return status;

  return status;
}

static int pvi_vlans_driver_node_add(
  uint32_t node_id,
  uint32_t new_node,
  uint32_t vlan_number)
{
  int clue_chip_filter_vlan = 1;
  uint32_t chip_bitmap      = 0;
  uint32_t node_exists      = 0;

  if (vlan_data[node_id][vlan_number].valid == false) return PV_INVALID_VLAN;

  /* Check if node exists */
  node_exists =
    vlan_data[node_id][vlan_number].vlan_mgr.nodes[0] & (1 << new_node);
  if (node_exists) return PV_EXISTS;

  /* Add node to Vlan data */
  vlan_data[node_id][vlan_number].vlan_mgr.nodes[0] |= 1 << new_node;

  /* Update CLUE Vlan ChipFilter Table */
  clue_readChipFilter(clue_chip_filter_vlan, vlan_number, &chip_bitmap);

  chip_bitmap |= 1 << new_node;

  clue_writeChipFilter(clue_chip_filter_vlan, vlan_number, chip_bitmap);

  return 0;
}

/** This function removes a node from a VLAN.
 * In Matrix it needs to configure the following tables:
 *    -REP VLAN filter table
 *    -EFEP Vlan filter table */
static int pvi_vlans_driver_node_remove(
  uint32_t node_id,
  uint32_t node_to_remove,
  uint32_t vlan_number)
{
  int clue_chip_filter_vlan = 1;
  uint32_t chip_bitmap      = 0;
  uint32_t node_exists      = 0;

  if (vlan_data[node_id][vlan_number].valid == false) return PV_INVALID_VLAN;

  /* Check if node exists */
  node_exists =
    vlan_data[node_id][vlan_number].vlan_mgr.nodes[0] & (1 << node_to_remove);
  if (!node_exists) return PV_NOT_FOUND;

  /* Remove node from Vlan data */
  vlan_data[node_id][vlan_number].vlan_mgr.nodes[0] &= ~(1 << node_to_remove);

  /* Update CLUE Vlan ChipFilter Table */
  clue_readChipFilter(clue_chip_filter_vlan, vlan_number, &chip_bitmap);

  chip_bitmap &= ~(1 << node_to_remove);

  clue_writeChipFilter(clue_chip_filter_vlan, vlan_number, chip_bitmap);

  return 0;
}

static int pvi_vlans_driver_vlan_exists(
  uint32_t node_id,
  uint32_t vlan_id,
  bool    *exists)
{
  if (vlan_data[node_id][vlan_id].valid) *exists = true;
  else *exists = false;

  return 0;
}

static int pvi_vlans_driver_members_get(
  uint32_t         node_id,
  uint32_t         vlan_number,
  struct pv_vport *vport,
  uint64_t        *nodes,
  uint32_t        *chassis)
{
  int status = 0;

  if (vlan_data[node_id][vlan_number].valid == false) return PV_INVALID_VLAN;

  if ((NULL == vport) && (NULL == nodes) && (NULL == chassis)) return PV_PARM;

  if (NULL != vport) {
    *vport = vlan_data[node_id][vlan_number].vlan_mgr.vlan_vports;
  }

  if (NULL != nodes) {
    *nodes = vlan_data[node_id][vlan_number].vlan_mgr.nodes[0];
  }

  if (NULL != chassis) {
    *chassis = vlan_data[node_id][vlan_number].vlan_mgr.chassis[0];
  }

  return status;
}

static int pvi_vlans_driver_mbv_lookup_enable_set(
  uint32_t               node_id,
  struct pv_vport        vport,
  bool                   enable,
  enum pv_vlans_mbv_type lookup_type,
  bool                   mac_sa_higher_pri)
{
  int status           = PV_OK;
  uint32_t port_number = 0;
  uint32_t read_value  = 0;

  // Get port number
  status = pv_vports_port_get(vport, &port_number);
  if (status) return status;

  // Read entry associated to the port, starting from the PORT_OFFSET_2
  status = pv_registerRead(REGID_NEO_HPP_PORT_OFFSET_2,
                           0,
                           port_number,
                           &read_value);
  if (status) return status;

  // Mask value to be enabled according to lookup type
  switch (lookup_type) {
  case PV_VLANS_MBV_TYPE_MAC_SA:
    FIELD_SET(&read_value,
              FIELDID_HPP_PORT_OFFSET_2_MBV_DO_MAC_SA_SEARCH,
              enable);
    break;

  case PV_VLANS_MBV_TYPE_IPV4_SA:
    FIELD_SET(&read_value,
              FIELDID_HPP_PORT_OFFSET_2_MBV_DO_IPV4_SA_SEARCH,
              enable);
    break;

  case PV_VLANS_MBV_TYPE_IPV6_SA:
    FIELD_SET(&read_value,
              FIELDID_HPP_PORT_OFFSET_2_MBV_DO_IPV6_SA_SEARCH,
              enable);
    break;

  default:
    return PV_PARM;
  }

  // Set the MAC SA with higher priority
  FIELD_SET(&read_value,
            FIELDID_HPP_PORT_OFFSET_2_MBV_MAC_SA_HIGHEST_PRIORITY,
            mac_sa_higher_pri);

  // Write in the register
  status = pv_registerWrite(REGID_NEO_HPP_PORT_OFFSET_2,
                            0,
                            port_number,
                            read_value);

  return status;
}

static int pvi_vlans_driver_mbv_mac_entry_set(
  uint32_t                   node_id,
  struct pv_vlans_mbv_mac    mac_data,
  struct pv_vlans_mbv_config entry_config,
  pv_uuid_t                  uuid)
{
  int status = PV_FEATURE_UNAVAILABLE;

  // TODO: UCX Matrix TCAM driver is not implemented yet.
  // It will be implemented in the User_story ID: 11217.
  // status = pvi_ucx_matrix_tcam__entry_write(Parameters to be defined);
  // Parameters will contain the node_id, mac_data, entry_config and uuid.

  return status;
}

static int pvi_vlans_driver_mbv_ip_sa_entry_set(
  uint32_t                   node_id,
  struct pv_vlans_mbv_ip     address_data,
  struct pv_vlans_mbv_config entry_config,
  pv_uuid_t                  uuid)
{
  int status = PV_FEATURE_UNAVAILABLE;

  // TODO: UCX Matrix TCAM driver is not implemented yet.
  // It will be implemented in the User_story ID: 11217.
  // status = pvi_ucx_matrix_tcam__entry_write(Parameters to be defined);
  // Parameters will contain the node_id, address_data, entry_config and uuid.

  return status;
}

static int pvi_vlans_driver_mbv_entry_get(
  uint32_t                    node_id,
  pv_uuid_t                   uuid,
  uint32_t                   *vid,
  struct pv_vlans_mbv_config *mbv_data)
{
  int status = PV_FEATURE_UNAVAILABLE;

  // TODO: UCX Matrix TCAM driver is not implemented yet.
  // It will be implemented in the User_story ID: 11217.
  // status = pvi_ucx_fender_tcam_entry_read(Parameters to be defined);
  // Parameters will contain the node_id, uuid, *vid and *mbv_data.

  return status;
}

static int pvi_vlans_driver_remap_table_vid_set(
  uint32_t node_id,
  uint32_t port_number,
  uint32_t old_vid,
  uint32_t new_vid)
{
  int status = 0;
  uint32_t read_value;

  if (PV_CHIP_PORT_MAX < port_number) return PV_OUT_OF_RANGE;

  asicReadOmVlanTagRam(port_number, old_vid, &read_value);

  read_value &= 0xFFFFF000;
  read_value |= new_vid;

  asicWriteOmVlanTagRam(port_number, old_vid, read_value);

  return status;
}

static int pvi_vlans_driver_remap_table_vid_get(
  uint32_t  node_id,
  uint32_t  port_number,
  uint32_t  old_vid,
  uint32_t *new_vid)
{
  int status = 0;

  if (PV_CHIP_PORT_MAX < port_number) return PV_OUT_OF_RANGE;

  if (NULL == new_vid) return PV_PARM;

  asicReadOmVlanTagRam(port_number, old_vid, new_vid);

  *new_vid &= 0x00000FFF;

  return status;
}

/* FLI driver interface to link with the driver instance */
static const struct pvi_vlans_mgr_driver_ops vlans_driver_ops =
{
  .initialize            = pvi_vlans_driver_initialize,
  .uninitialize          = pvi_vlans_driver_uninitialize,
  .vlan_create           = pvi_vlans_driver_vlan_create,
  .vlan_remove           = pvi_vlans_driver_vlan_remove,
  .illegal_vlan_set      = pvi_vlans_driver_illegal_vlan_set,
  .illegal_vlan_get      = pvi_vlans_driver_illegal_vlan_get,
  .vid_to_vlan_set       = pvi_vlans_driver_vid_to_vlan_set,
  .vid_to_vlan_get       = pvi_vlans_driver_vid_to_vlan_get,
  .vlan_to_vid_set       = pvi_vlans_driver_vlan_to_vid_set,
  .vlan_to_vid_get       = pvi_vlans_driver_vlan_to_vid_get,
  .port_add              = pvi_vlans_driver_port_add,
  .port_remove           = pvi_vlans_driver_port_remove,
  .node_add              = pvi_vlans_driver_node_add,
  .node_remove           = pvi_vlans_driver_node_remove,
  .chassis_add           = NULL,
  .chassis_remove        = NULL,
  .port_vid_set          = pvi_vlans_driver_port_vid_set,
  .protocol_vid_get      = pvi_vlans_driver_protocol_vid_get,
  .protocol_vid_set      = pvi_vlans_driver_protocol_vid_set,
  .protocol_qos_get      = NULL,
  .protocol_qos_set      = NULL,
  .protocol_qos_remove   = NULL,
  .hw_initialize         = NULL,
  .mstp_port_state_set   = pvi_vlans_driver_mstp_port_state_set,
  .mstp_port_state_get   = pvi_vlans_driver_mstp_port_state_get,
  .vlan_exists           = pvi_vlans_driver_vlan_exists,
  .members_get           = pvi_vlans_driver_members_get,
  .mbv_lookup_enable_set = pvi_vlans_driver_mbv_lookup_enable_set,
  .mbv_mac_entry_set     = pvi_vlans_driver_mbv_mac_entry_set,
  .mbv_ip_sa_entry_set   = pvi_vlans_driver_mbv_ip_sa_entry_set,
  .mbv_entry_get         = pvi_vlans_driver_mbv_entry_get,
  .remap_table_vid_set   = pvi_vlans_driver_remap_table_vid_set,
  .remap_table_vid_get   = pvi_vlans_driver_remap_table_vid_get
};

/*  driver instance to register to the FLI manager */
static const struct pvi_vlans_mgr_driver_interface vlans_driver_interface =
{
  .name = VLANS_DRIVER_NAME,
  .arch = PVI_ARCH_BTTF_PACUARE,
  .ops  = &vlans_driver_ops,
};


static int pvi_vlans_driver_probe(
  struct  pvi_driver_instance *driver_instance) {
  int status = 0;

  // Check if the driver_instance is NULL
  if (NULL == driver_instance) return PV_PARM;

  // Check driver name is not null
  if (NULL == driver_instance->driver->name) return PV_PARM;


  // Ensure the driver_instance is the same type this driver is
  if (strcmp(driver_instance->driver->name, VLANS_DRIVER_NAME)) return PV_BAD_ID;

  /* Register to the vlans manager */
  status = pvi_vlans_mgr_driver_register(&vlans_driver_interface);
  if (status) return status;


  memset(&vlan_data, 0, sizeof(struct pvi_matrix_vlan_data));

  return status;
}

static int pvi_vlans_driver_remove(
  struct  pvi_driver_instance *driver_instance) {
  int status = 0;

  // Check if the driver_instance is NULL
  if (NULL == driver_instance) return PV_PARM;

  // Check driver name is not null
  if (NULL == driver_instance->driver->name) return PV_PARM;

  // Ensure the driver_instance is the same type this driver is
  if (strcmp(driver_instance->driver->name, VLANS_DRIVER_NAME)) return PV_BAD_ID;

  /* Remove from vlans manager */
  status = pvi_vlans_mgr_driver_remove(&vlans_driver_interface);
  if (status) return status;

  driver_instance->priv = NULL;

  return status;
}

static const struct pvi_driver_instance_id
  pvi_vlans_driver_instance_ids[] =
{
  { .compatible = "pvi_matrix_vlans", .data = NULL },
  {}
};

static struct pvi_driver vlans_driver = {
  .name                = VLANS_DRIVER_NAME,
  .driver_instance_ids = pvi_vlans_driver_instance_ids,
  .probe               = pvi_vlans_driver_probe,
  .remove              = pvi_vlans_driver_remove
};

PVI_DRIVER_INIT(vlans_driver, "pvi_vlans")

