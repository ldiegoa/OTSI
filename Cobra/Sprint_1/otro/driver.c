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
#include <legacy/pv/asic/asic_specific.h>
#include <legacy/prim.p/primIpp.h>
#include <provision/internal/core/pvi_driver.h>
#include \
  <provision/internal/fli/network_ports/network_ports_subsystem/pvi_network_ports_mgr.h>
#include <stdbool.h>

// D E F I N E S

/** Network ports driver name */
#define NETWORK_PORTS_DRIVER_NAME "matrix_network_ports_driver"

/** Number of front plane ports */
#define FRONT_PLANE_PORTS ARCH_CHIPPORT_NUMBER

/** Min number of front plane ports */
#define MIN_FRONT_PLANE_PORTS 2

/** Max number of front plane ports */
#define MAX_FRONT_PLANE_PORTS \
  (PV_CHIP_PORT_MAX - MIN_FRONT_PLANE_PORTS)

/** MAX frame size */
#define MAX_FRAME_SIZE 9600

/** Source port precedence default value */
#define SRC_PORT_PRECEDENCE_DEF 1

/** Ingress logport precedence default value */
#define IN_LOGPORT_PRECEDENCE_DEF 1


/*
 * Supports:
 * Conduit/OOBM Port (C), DMA Port (D), 1 Gb/s Port (G) for PACUARE
 * Conduit Port (C), DMA Port (D) for NEO
 */
#define SUPPORTED_SPEEDS_CDG PV_PORT_SPEED_10M | \
  PV_PORT_SPEED_100M | PV_PORT_SPEED_1G

/*
 * Supports:
 * Recirculation Port (R) for PACUARE and NEO
 */
#define SUPPORTED_SPEEDS_R PV_PORT_SPEED_MAX

/*
 * Supports:
 * 40 Gb/s Port (XL) for PACUARE and NEO
 */
#define SUPPORTED_SPEEDS_XL    PV_PORT_SPEED_40G

/*
 * Supports:
 * Fast Connect Port (FC) for PACUARE
 * Fast Connect Port (X) for NEO
 */
#define SUPPORTED_SPEEDS_FC_X           PV_PORT_SPEED_1G | PV_PORT_SPEED_2P5G | \
  PV_PORT_SPEED_5G | PV_PORT_SPEED_10G

/*
 * Supports:
 * 10 Gb/s Port (X) for PACUARE
 */
#define SUPPORTED_SPEEDS_X              PV_PORT_SPEED_10G

/*
 * Supports:
 * OLA Monitor Port (M) for NEO
 */
#define SUPPORTED_SPEEDS_M              PV_PORT_SPEED_1G | PV_PORT_SPEED_2P5G | \
  PV_PORT_SPEED_10G

/*
 * Supports:
 * Fast Connect Port (G') for NEO
 */
#define SUPPORTED_SPEEDS_GP             PV_PORT_SPEED_1G | PV_PORT_SPEED_2P5G

/*
 * Supports:
 * Recirculation Port (R), 40 Gb/s Port (XL) for PACUARE
 */
#define SUPPORTED_SPEEDS_R_XL           PV_PORT_SPEED_MAX | PV_PORT_SPEED_40G

/*
 * Supports:
 * Fast Connect Port (FC), 1 Gb/s Port (G) for PACUARE
 */
#define SUPPORTED_SPEEDS_FC_G           PV_PORT_SPEED_10M |    \
  PV_PORT_SPEED_100M | PV_PORT_SPEED_1G | PV_PORT_SPEED_2P5G | \
  PV_PORT_SPEED_5G | PV_PORT_SPEED_10G

/*
 * Supports:
 * Recirculation Port (R), OLA Monitor Port (M), Fast Connect Port(G') for NEO
 */
#define SUPPORTED_SPEEDS_RMGP           PV_PORT_SPEED_MAX | PV_PORT_SPEED_1G \
  | PV_PORT_SPEED_2P5G | PV_PORT_SPEED_10G

/*
 * Supports:
 * Fast Connect Port(G') and 1 Gb/sPort (G) for NEO
 */
#define SUPPORTED_SPEEDS_GGP            PV_PORT_SPEED_10M | PV_PORT_SPEED_100M \
  | PV_PORT_SPEED_1G | PV_PORT_SPEED_2P5G

/*
 * Supports Recirculation Port (R), 40 Gb/s Port (XL) for NEO and PACUARE
 */
#define SUPPORTED_SPEEDS_RXL            PV_PORT_SPEED_MAX | PV_PORT_SPEED_40G

/*
 * Supports:
 * OLA Monitor Port (M), Fast Connect Port(G') for NEO
 */
#define SUPPORTED_SPEEDS_XMGP           PV_PORT_SPEED_1G | PV_PORT_SPEED_2P5G | \
  PV_PORT_SPEED_5G | PV_PORT_SPEED_10G

// E N U M E R A T I O N S

/**
 * Supported speeds per port. It's used to make sure that a port supports a
 * specific speed before setting it.
 */
enum pv_port_speed *available_speed_masks = NULL;

/** Supported speeds for pacuare arch */
enum pv_port_speed pacuare_available_speed_masks[60] = {
  SUPPORTED_SPEEDS_CDG,  /* Port 0 */
  SUPPORTED_SPEEDS_CDG,  /* Port 1 */
  SUPPORTED_SPEEDS_RXL,  /* Port 2 */
  SUPPORTED_SPEEDS_R,    /* Port 3 */
  SUPPORTED_SPEEDS_R,    /* Port 4 */
  SUPPORTED_SPEEDS_R,    /* Port 5 */
  SUPPORTED_SPEEDS_X,    /* Port 6 */
  SUPPORTED_SPEEDS_X,    /* Port 7 */
  SUPPORTED_SPEEDS_X,    /* Port 8 */
  SUPPORTED_SPEEDS_X,    /* Port 9 */
  SUPPORTED_SPEEDS_FC_G, /* Port 10 */
  SUPPORTED_SPEEDS_FC_G, /* Port 11 */
  SUPPORTED_SPEEDS_FC_G, /* Port 12 */
  SUPPORTED_SPEEDS_FC_G, /* Port 13 */
  SUPPORTED_SPEEDS_FC_G, /* Port 14 */
  SUPPORTED_SPEEDS_FC_G, /* Port 15 */
  SUPPORTED_SPEEDS_FC_G, /* Port 16 */
  SUPPORTED_SPEEDS_FC_G, /* Port 17 */
  SUPPORTED_SPEEDS_CDG,  /* Port 18 */
  SUPPORTED_SPEEDS_CDG,  /* Port 19 */
  SUPPORTED_SPEEDS_CDG,  /* Port 20 */
  SUPPORTED_SPEEDS_CDG,  /* Port 21 */
  SUPPORTED_SPEEDS_CDG,  /* Port 22 */
  SUPPORTED_SPEEDS_CDG,  /* Port 23 */
  SUPPORTED_SPEEDS_CDG,  /* Port 24 */
  SUPPORTED_SPEEDS_CDG,  /* Port 25 */
  SUPPORTED_SPEEDS_CDG,  /* Port 26 */
  SUPPORTED_SPEEDS_CDG,  /* Port 27 */
  SUPPORTED_SPEEDS_CDG,  /* Port 28 */
  SUPPORTED_SPEEDS_CDG,  /* Port 29 */
  SUPPORTED_SPEEDS_CDG,  /* Port 30 */
  SUPPORTED_SPEEDS_CDG,  /* Port 31 */
  SUPPORTED_SPEEDS_CDG,  /* Port 32 */
  SUPPORTED_SPEEDS_CDG,  /* Port 33 */
  SUPPORTED_SPEEDS_CDG,  /* Port 34 */
  SUPPORTED_SPEEDS_CDG,  /* Port 35 */
  SUPPORTED_SPEEDS_CDG,  /* Port 36 */
  SUPPORTED_SPEEDS_CDG,  /* Port 37 */
  SUPPORTED_SPEEDS_CDG,  /* Port 38 */
  SUPPORTED_SPEEDS_CDG,  /* Port 39 */
  SUPPORTED_SPEEDS_CDG,  /* Port 40 */
  SUPPORTED_SPEEDS_CDG,  /* Port 41 */
  SUPPORTED_SPEEDS_CDG,  /* Port 42 */
  SUPPORTED_SPEEDS_CDG,  /* Port 43 */
  SUPPORTED_SPEEDS_CDG,  /* Port 44 */
  SUPPORTED_SPEEDS_CDG,  /* Port 45 */
  SUPPORTED_SPEEDS_CDG,  /* Port 46 */
  SUPPORTED_SPEEDS_CDG,  /* Port 47 */
  SUPPORTED_SPEEDS_CDG,  /* Port 48 */
  SUPPORTED_SPEEDS_CDG,  /* Port 49 */
  SUPPORTED_SPEEDS_CDG,  /* Port 50 */
  SUPPORTED_SPEEDS_CDG,  /* Port 51 */
  SUPPORTED_SPEEDS_CDG,  /* Port 52 */
  SUPPORTED_SPEEDS_CDG,  /* Port 53 */
  SUPPORTED_SPEEDS_CDG,  /* Port 54 */
  SUPPORTED_SPEEDS_CDG,  /* Port 55 */
  SUPPORTED_SPEEDS_CDG,  /* Port 56 */
  SUPPORTED_SPEEDS_CDG,  /* Port 57 */
  SUPPORTED_SPEEDS_CDG,  /* Port 58 */
  SUPPORTED_SPEEDS_CDG   /* Port 59 */
};

/** Supported speeds for neo arch */
enum pv_port_speed neo_available_speed_masks[36] = {
  SUPPORTED_SPEEDS_CDG,  /* Port 0 */
  SUPPORTED_SPEEDS_CDG,  /* Port 1 */
  SUPPORTED_SPEEDS_RMGP, /* Port 2 */
  SUPPORTED_SPEEDS_RMGP, /* Port 3 */
  SUPPORTED_SPEEDS_RXL,  /* Port 4 */
  SUPPORTED_SPEEDS_RXL,  /* Port 5 */
  SUPPORTED_SPEEDS_XMGP, /* Port 6 */
  SUPPORTED_SPEEDS_XMGP, /* Port 7 */
  SUPPORTED_SPEEDS_XMGP, /* Port 8 */
  SUPPORTED_SPEEDS_XMGP, /* Port 9 */
  SUPPORTED_SPEEDS_FC_G, /* Port 10 */
  SUPPORTED_SPEEDS_FC_G, /* Port 11 */
  SUPPORTED_SPEEDS_FC_G, /* Port 12 */
  SUPPORTED_SPEEDS_FC_G, /* Port 13 */
  SUPPORTED_SPEEDS_CDG,  /* Port 14 */
  SUPPORTED_SPEEDS_CDG,  /* Port 15 */
  SUPPORTED_SPEEDS_CDG,  /* Port 16 */
  SUPPORTED_SPEEDS_CDG,  /* Port 17 */
  SUPPORTED_SPEEDS_CDG,  /* Port 18 */
  SUPPORTED_SPEEDS_CDG,  /* Port 19 */
  SUPPORTED_SPEEDS_CDG,  /* Port 20 */
  SUPPORTED_SPEEDS_CDG,  /* Port 21 */
  SUPPORTED_SPEEDS_CDG,  /* Port 22 */
  SUPPORTED_SPEEDS_CDG,  /* Port 23 */
  SUPPORTED_SPEEDS_CDG,  /* Port 24 */
  SUPPORTED_SPEEDS_CDG,  /* Port 25 */
  SUPPORTED_SPEEDS_GGP,  /* Port 26 */
  SUPPORTED_SPEEDS_GGP,  /* Port 27 */
  SUPPORTED_SPEEDS_GGP,  /* Port 28 */
  SUPPORTED_SPEEDS_GGP,  /* Port 29 */
  SUPPORTED_SPEEDS_GGP,  /* Port 30 */
  SUPPORTED_SPEEDS_GGP,  /* Port 31 */
  SUPPORTED_SPEEDS_GGP,  /* Port 32 */
  SUPPORTED_SPEEDS_GGP,  /* Port 33 */
  SUPPORTED_SPEEDS_CDG,  /* Port 34 */
  SUPPORTED_SPEEDS_CDG   /* Port 35 */
};

// S T R U C T U R E S

/** Network ports driver private data per node */
struct pvi_network_ports_driver_data {
  /** Configured speeds per enabled port */
  enum pv_port_speed speed_per_port[FRONT_PLANE_PORTS];

  /** Loopback enable status per port*/
  enum pv_port_loopback_mode ena_loopback_per_port[FRONT_PLANE_PORTS];
};

// P R I V A T E   D A T A

// F U N C T I O N S
static int pvi_network_ports_matrix_driver_node_initialize(
  uint32_t                                      node_id,
  struct pvi_network_ports_mgr_driver_instance *driver_instance,
  bool                                          warmboot)
{
  struct pvi_network_ports_driver_data *driver_data = NULL;

  if ((NULL == driver_instance) ||
      (NULL == driver_instance->subpool)) return PV_PARM;

  // Create available_speed_masks
  available_speed_masks = pvi_mallocz(node_id,
                                      sizeof(enum pv_port_speed) *
                                      FRONT_PLANE_PORTS);
  if (NULL == available_speed_masks) return PV_MEMORY;

  // Fill available_speed_masks according the architecture
  if (PVI_ARCH_BTTF_PACUARE == driver_instance->interface->arch) {
    memcpy(available_speed_masks, pacuare_available_speed_masks,
           sizeof(enum pv_port_speed) * FRONT_PLANE_PORTS);
  } else {
    memcpy(available_speed_masks, neo_available_speed_masks,
           sizeof(enum pv_port_speed) * FRONT_PLANE_PORTS);
  }

  driver_data =
    pvi_mallocz(node_id, sizeof(struct pvi_network_ports_driver_data));
  if (NULL == driver_data) return PV_MEMORY;

  driver_instance->priv      = (void *)driver_data;
  driver_instance->priv_size = sizeof(struct pvi_network_ports_driver_data);

  for (int chipPort = 0; chipPort < ARCH_CHIPPORT_NUMBER; chipPort++) {
    uint32_t regData = 0;

    /* MIPMISC Configuration */
    asicMipmiscReadReg32(0, NG_MIPMISC_MAC_CONFIG, chipPort, &regData);
    asicSetFieldToData(0, NG_MIPMISC_MAC_CONFIG, MAC_MAC_RESET_L, 0x1, &regData);
    asicMipmiscWriteReg32(0, NG_MIPMISC_MAC_CONFIG, chipPort, regData);
  }

  return PV_OK;
}

static int pvi_network_ports_matrix_driver_node_uninitialize(
  uint32_t node_id,
  void    *priv)
{
  pvi_free(node_id, available_speed_masks);

  return PV_OK;
}

static int pvi_network_ports_matrix_driver_resources_read(
  uint32_t                                node_id,
  void                                   *priv,
  struct pvi_network_ports_mgr_resources *resources)
{
  if (NULL == resources) return PV_PARM;

  resources->ports_number    = PV_CHIP_PORT_MAX;
  resources->fp_ports_number = MAX_FRONT_PLANE_PORTS;
  resources->sp_ports_start  = FRONT_PLANE_PORTS;
  resources->sp_ports_number = 0;

  return PV_OK;
}

static int pvi_network_ports_matrix_driver_capabilities_get(
  uint32_t                              node_id,
  void                                 *priv,
  struct pv_vport                       port,
  struct pv_network_ports_capabilities *capabilities)
{
  int status       = 0;
  uint32_t port_id = 0;

  status = pv_vports_port_get(port, &port_id);
  if (status) return status;

  if ((MAX_FRONT_PLANE_PORTS < port_id) ||
      (MIN_FRONT_PLANE_PORTS > port_id)) {
    return PV_OUT_OF_RANGE;
  }

  memset(capabilities, 0, sizeof(struct pv_network_ports_capabilities));
  capabilities->speeds         = available_speed_masks[port_id];
  capabilities->fc_capable     = true;
  capabilities->eee_capable    = true;
  capabilities->max_frame_size = MAX_FRAME_SIZE;

  return 0;
}

static int pvi_network_ports_matrix_driver_init(
  uint32_t                               node_id,
  void                                  *priv,
  struct pv_vport                        port,
  struct pv_network_ports_configuration *config)
{
  config->src_port_precedence   = SRC_PORT_PRECEDENCE_DEF;
  config->in_logport_precedence = IN_LOGPORT_PRECEDENCE_DEF;

  return PV_OK;
}

static int pvi_network_ports_matrix_driver_autoneg_config_set(
  uint32_t        node_id,
  void           *priv,
  struct pv_vport port,
  uint32_t        an_feat_mask)
{
  return PV_FEATURE_UNAVAILABLE;
}

static int pvi_network_ports_matrix_driver_autoneg_enable(
  uint32_t        node_id,
  void           *priv,
  struct pv_vport port,
  bool            an_enable)
{
  return PV_FEATURE_UNAVAILABLE;
}

static int pvi_network_ports_matrix_driver_speed_set(
  uint32_t           node_id,
  void              *priv,
  struct pv_vport    port,
  enum pv_port_speed speed)
{
  int status       = 0;
  uint32_t port_id = 0;

  // uint8_t  pg      = 0;
  // uint8_t  slice   = 0;

  // uint8_t  slice_port = 0;
  // uint32_t port_idx   = 0;

  struct pvi_network_ports_driver_data *driver_data =
    (struct pvi_network_ports_driver_data *)priv;

  if (NULL == driver_data) return -EFAULT;

  status = pv_vports_port_get(port, &port_id);
  if (status) return status;

  // TODO: Mariano please finish this implementation

  return PV_FEATURE_UNAVAILABLE;
}

static int pvi_network_ports_matrix_driver_max_frame_size_set(
  uint32_t           node_id,
  void              *priv,
  struct pv_vport    port,
  enum pv_port_speed speed,
  uint32_t           max_frame_size)
{
  return PV_FEATURE_UNAVAILABLE;
}

static int pvi_network_ports_matrix_driver_eee_config_set(
  uint32_t                           node_id,
  void                              *priv,
  struct pv_vport                    port,
  enum pv_port_speed                 speed,
  struct pv_network_ports_eee_config eee_config)
{
  return PV_FEATURE_UNAVAILABLE;
}

static int pvi_network_ports_matrix_driver_flow_ctrl_mode_set(
  uint32_t             node_id,
  void                *priv,
  struct pv_vport      port,
  enum pv_port_speed   speed,
  enum pv_port_fc_mode fc_mode)
{
  return PV_FEATURE_UNAVAILABLE;
}

static int pvi_network_ports_matrix_driver_config_get(
  uint32_t                               node_id,
  void                                  *priv,
  struct pv_vport                        port,
  struct pv_network_ports_configuration *config)
{
  return PV_FEATURE_UNAVAILABLE;
}

static int pvi_network_ports_matrix_driver_config_set(
  uint32_t                              node_id,
  void                                 *priv,
  struct pv_vport                       port,
  struct pv_network_ports_configuration config)
{
  return PV_FEATURE_UNAVAILABLE;
}

static int pvi_network_ports_matrix_driver_sec_config_get(
  uint32_t                                   node_id,
  void                                      *priv,
  struct pv_vport                            port,
  struct pv_network_ports_sec_configuration *sec_config)
{
  return PV_FEATURE_UNAVAILABLE;
}

static int pvi_network_ports_matrix_driver_sec_config_set(
  uint32_t                                  node_id,
  void                                     *priv,
  struct pv_vport                           port,
  struct pv_network_ports_sec_configuration sec_config)
{
  return PV_FEATURE_UNAVAILABLE;
}

static int pvi_network_ports_matrix_driver_led_config_set(
  uint32_t                           node_id,
  void                              *priv,
  struct pv_vport                    port,
  struct pv_network_ports_led_config led_config)
{
  return PV_FEATURE_UNAVAILABLE;
}

static int pvi_network_ports_matrix_driver_led_config_get(
  uint32_t                            node_id,
  void                               *priv,
  struct pv_vport                     port,
  struct pv_network_ports_led_config *led_config)
{
  return PV_FEATURE_UNAVAILABLE;
}

static int pvi_network_ports_matrix_driver_loopback_mode_set(
  uint32_t                   node_id,
  void                      *priv,
  struct pv_vport            port,
  enum pv_port_speed         speed,
  enum pv_port_loopback_mode lb_mode)
{
  return PV_FEATURE_UNAVAILABLE;
}

static int pvi_network_ports_matrix_driver_recirculation_enable(
  uint32_t        node_id,
  void           *priv,
  struct pv_vport port,
  bool            recirc_enable)
{
  int status = 0;
  int ret;
  HW_LOGPORT logPort;
  uint32_t   regData;
  uint32_t   twoWordData[2];
  uint32_t  *counterData;
  uint32_t   chipPort;

  status = pv_vports_port_get(port, &chipPort);
  if (status) return status;

  if (recirc_enable) {
    /* MIPMISC Configuration */
    asicMipmiscReadReg32(thisCpu, NG_MIPMISC_MAC_CONFIG, (CHIP_PORT_t)chipPort,
                         &regData);
    asicSetFieldToData(0, NG_MIPMISC_MAC_CONFIG, MAC_MAC_SPEED_s2MAC, 0x7,
                       &regData);
    asicMipmiscWriteReg32(thisCpu, NG_MIPMISC_MAC_CONFIG, chipPort,
                          regData);

    asicMipmiscReadReg32(thisCpu, NG_MIPMISC_MAC_CONFIG, chipPort,
                         &regData);
    asicSetFieldToData(0, NG_MIPMISC_MAC_CONFIG, MAC_MAC_RESET_L, 0x1,
                       &regData);
    asicMipmiscWriteReg32(thisCpu, NG_MIPMISC_MAC_CONFIG, chipPort,
                          regData);

    /* MAC Configuration*/
    twoWordData[0] = 0;
    twoWordData[1] = 0;
    asicWriteMem(thisCpu, NEO_MAC_RECIRC_PORT_SRC_ADDR_RECIRC_STATIC,
                 chipPort, &twoWordData[0]);

    regData = 0;
    asicSetFieldToData(thisCpu, NEO_MAC_RECIRC_PORT_RESET_D3BA,
                       D3BA_PKR_EN, 1, &regData);
    asicSetFieldToData(thisCpu, NEO_MAC_RECIRC_PORT_RESET_D3BA,
                       D3BA_OFP_RESET_L, 1, &regData);
    asicSetFieldToData(thisCpu, NEO_MAC_RECIRC_PORT_RESET_D3BA,
                       D3BA_UNPKR_RESET_L, 1, &regData);
    asicSetFieldToData(thisCpu, NEO_MAC_RECIRC_PORT_RESET_D3BA,
                       D3BA_CP_RESET_L, 1, &regData);
    asicWriteReg32(thisCpu, NEO_MAC_RECIRC_PORT_RESET_D3BA, chipPort,
                   regData);

    asicWriteField32(thisCpu, NEO_MAC_RECIRC_DOUBLEQ_TAG_CTRL_D3BA_STATIC,
                     chipPort, STATIC_USE_CFI_AS_DEI_IN_CTAG, 1);

    /* Add FPS_ENCAPS_SIZE to MAX_UNTAGGED_FRAME_SIZE to make sure that
     * FPS packets aren't dropped when they go through the recirc
     * port.  If a packet it too big, it will have been filtered
     * when the packet first ingressed into the ASIC. */
    asicWriteField32(thisCpu, NEO_MAC_RECIRC_MAX_PKT_LEN_RECIRC_STATIC,
                     chipPort, MAX_PKT_LEN,
                #ifdef FTR_FPS
                     FPS_ENCAPS_SIZE +
                #endif /* FTR_FPS */
                     MAX_UNTAGGED_FRAME_SIZE);

    /* IPP Configuration */
    hw_ipp_recircPortInit(chipPort);

    /* Program HPP and FR logport */
    logPort = (HW_LOGPORT_TRUNK_BIT | HW_TUF_RECIRC_TRUNK_IDX);

    ret = portRegs_logPortWrite(thisCpu, chipPort, logPort, 0);
    if (ret != 0) ASSERT(ret == 0);

    /* Program HPP */
    portRegs_StackRecircPortWrite(thisCpu, chipPort, PORT_REGS_SR_RECIRC, 1, 0);

    /* Program FR */
    portRegs_egressWrite(0, chipPort, PORT_REGS_EGRESS_PORT_DISABLE, 0, 0);
    portRegs_egressWrite(0, chipPort, PORT_REGS_EGRESS_JUMBO,        1, 0);

    /* Program Counters */
    counterData = (uint32_t *)malloc(2048 * sizeof(uint32_t));
    ASSERT(counterData != NULL);

    /* clear this memory segment */
    memset(counterData, 0, 2048 * sizeof(uint32_t));

    asicWriteMem(thisCpu, NEO_MAC_RECIRC_CP_MEM_OFFSET_CS_P, chipPort,
                 counterData);

    asicWriteReg32(0, NEO_MAC_RECIRC_QUEUE_PAUSED_CNTR_OFFSET_P, chipPort, 0);

    free(counterData);

    return PV_OK;
  } else {
    /* Program FR */
    portRegs_egressWrite(0, chipPort, PORT_REGS_EGRESS_PORT_DISABLE, 1, 0);

    /* MAC Configuration*/
    regData = 0;
    asicSetFieldToData(thisCpu, NEO_MAC_RECIRC_PORT_RESET_D3BA,
                       D3BA_PKR_EN, 0, &regData);
    asicSetFieldToData(thisCpu, NEO_MAC_RECIRC_PORT_RESET_D3BA,
                       D3BA_OFP_RESET_L, 0, &regData);
    asicSetFieldToData(thisCpu, NEO_MAC_RECIRC_PORT_RESET_D3BA,
                       D3BA_UNPKR_RESET_L, 0, &regData);
    asicSetFieldToData(thisCpu, NEO_MAC_RECIRC_PORT_RESET_D3BA,
                       D3BA_CP_RESET_L, 0, &regData);
    asicWriteReg32(thisCpu, NEO_MAC_RECIRC_PORT_RESET_D3BA, chipPort,
                   regData);

    /* MIPMISC Configuration */
    asicMipmiscReadReg32(thisCpu, NG_MIPMISC_MAC_CONFIG, chipPort,
                         &regData);
    asicSetFieldToData(0, NG_MIPMISC_MAC_CONFIG, MAC_MAC_RESET_L, 0x0,
                       &regData);
    asicMipmiscWriteReg32(thisCpu, NG_MIPMISC_MAC_CONFIG, chipPort,
                          regData);
    return PV_OK;
  }
}

static int pvi_network_ports_matrix_driver_link_enable(
  uint32_t           node_id,
  void              *priv,
  struct pv_vport    port,
  enum pv_port_speed speed,
  bool               enable)
{
  int status       = 0;
  uint32_t port_id = 0;

  struct pvi_network_ports_driver_data *driver_data =
    (struct pvi_network_ports_driver_data *)priv;

  if (NULL == driver_data) return PV_PARM;

  status = pv_vports_port_get(port, &port_id);
  if (status) return status;

  // TODO: Mariano please finish this implementation
  return PV_FEATURE_UNAVAILABLE;
}

static int pvi_network_ports_matrix_driver_enable(
  uint32_t           node_id,
  void              *priv,
  struct pv_vport    port,
  enum pv_port_speed speed,
  bool               tx_enable,
  bool               rx_enable)
{
  int status               = 0;
  uint32_t port_id         = 0;
  uint32_t fp_ports_number = FRONT_PLANE_PORTS;

  struct pvi_network_ports_driver_data *driver_data =
    (struct pvi_network_ports_driver_data *)priv;

  // struct pvi_ucx_pg_mgr_port_config port_config = { 0 };

  if (NULL == driver_data) return -EFAULT;

  status = pv_vports_port_get(port, &port_id);
  if (status) return status;

  if (fp_ports_number > port_id) {
    // Check if the speed is available for the port
    if (0 == (available_speed_masks[port_id] & speed)) return -EINVAL;
  }
  return PV_FEATURE_UNAVAILABLE;
}

static int pvi_network_ports_matrix_driver_link_status_get(
  uint32_t           node_id,
  void              *priv,
  struct pv_vport    port,
  enum pv_port_speed speed,
  bool              *link)
{
  return PV_FEATURE_UNAVAILABLE;
}

static int pvi_network_ports_matrix_driver_status_get(
  uint32_t                        node_id,
  void                           *priv,
  struct pv_vport                 port,
  enum pv_port_speed              speed,
  struct pv_network_ports_status *status)
{
  int e_status     = 0;
  uint32_t port_id = 0;

  e_status = pv_vports_port_get(port, &port_id);
  if (e_status) return e_status;

  if ((MAX_FRONT_PLANE_PORTS < port_id) ||
      (MIN_FRONT_PLANE_PORTS > port_id)) {
    return PV_OUT_OF_RANGE;
  }

  return PV_FEATURE_UNAVAILABLE;
}

static int pvi_network_ports_matrix_driver_counter_clear(
  uint32_t             node_id,
  void                *priv,
  struct pv_vport      port,
  enum pv_port_counter counter)
{
  return PV_FEATURE_UNAVAILABLE;
}

static int pvi_network_ports_matrix_driver_counter_clear_all(
  uint32_t        node_id,
  void           *priv,
  struct pv_vport port)
{
  return PV_FEATURE_UNAVAILABLE;
}

static int pvi_network_ports_matrix_driver_counter_read(
  uint32_t             node_id,
  void                *priv,
  struct pv_vport      port,
  enum pv_port_counter counter,
  uint64_t            *counter_value)
{
  return PV_FEATURE_UNAVAILABLE;
}

static int pvi_network_ports_matrix_driver_vlan_tag_mode_set(
  uint32_t                            node_id,
  void                               *priv,
  struct pv_vport                     port,
  enum pv_network_ports_vlan_tag_mode tag_mode)
{
  return PV_FEATURE_UNAVAILABLE;
}

static int pvi_network_ports_matrix_driver_vlan_tag_mode_get(
  uint32_t                             node_id,
  void                                *priv,
  struct pv_vport                      port,
  enum pv_network_ports_vlan_tag_mode *tag_mode)
{
  return PV_FEATURE_UNAVAILABLE;
}

static int pvi_network_ports_matrix_driver_link_monitor_set(
  uint32_t                                      node_id,
  struct pvi_network_ports_mgr_driver_instance *driver_instance,
  struct pv_vport                               port,
  bool                                          enable)
{
  return PV_FEATURE_UNAVAILABLE;
}

static int pvi_network_ports_matrix_driver_link_monitor_get(
  uint32_t         node_id,
  void            *priv,
  struct pv_vport *port)
{
  return PV_FEATURE_UNAVAILABLE;
}

static int pvi_network_ports_matrix_driver_link_monitor_callback_register(
  uint32_t                        node_id,
  void                           *priv,
  pv_network_port_link_callback_t callback)
{
  return PV_FEATURE_UNAVAILABLE;
}

static int pvi_network_ports_matrix_driver_sampling_enable(
  uint32_t node_id,
  void    *priv,
  bool     sampling_enable)
{
  return PV_FEATURE_UNAVAILABLE;
}

static int pvi_network_ports_matrix_driver_sampling_port_config_set(
  uint32_t                                       node_id,
  void                                          *priv,
  struct pv_vport                                port,
  struct pv_network_ports_sampling_configuration configuration)
{
  return PV_FEATURE_UNAVAILABLE;
}

static int pvi_network_ports_matrix_driver_sampling_port_config_get(
  uint32_t                                        node_id,
  void                                           *priv,
  struct pv_vport                                 port,
  struct pv_network_ports_sampling_configuration *configuration)
{
  return PV_FEATURE_UNAVAILABLE;
}

static const struct pvi_network_ports_mgr_driver_ops network_ports_driver_ops =
{
  .mgr_node_initialize            =
    pvi_network_ports_matrix_driver_node_initialize,
  .mgr_node_uninitialize          =
    pvi_network_ports_matrix_driver_node_uninitialize,
  .resources_read                 =
    pvi_network_ports_matrix_driver_resources_read,
  .capabilities_get               =
    pvi_network_ports_matrix_driver_capabilities_get,
  .init               = pvi_network_ports_matrix_driver_init,
  .autoneg_config_set =
    pvi_network_ports_matrix_driver_autoneg_config_set,
  .autoneg_enable                 =
    pvi_network_ports_matrix_driver_autoneg_enable,
  .speed_set          = pvi_network_ports_matrix_driver_speed_set,
  .max_frame_size_set =
    pvi_network_ports_matrix_driver_max_frame_size_set,
  .eee_config_set                 =
    pvi_network_ports_matrix_driver_eee_config_set,
  .flow_ctrl_mode_set             =
    pvi_network_ports_matrix_driver_flow_ctrl_mode_set,
  .config_get     = pvi_network_ports_matrix_driver_config_get,
  .config_set     = pvi_network_ports_matrix_driver_config_set,
  .sec_config_get =
    pvi_network_ports_matrix_driver_sec_config_get,
  .sec_config_set                 =
    pvi_network_ports_matrix_driver_sec_config_set,
  .led_config_set                 =
    pvi_network_ports_matrix_driver_led_config_set,
  .led_config_get                 =
    pvi_network_ports_matrix_driver_led_config_get,
  .loopback_mode_set              =
    pvi_network_ports_matrix_driver_loopback_mode_set,
  .recirculation_enable           =
    pvi_network_ports_matrix_driver_recirculation_enable,
  .link_enable     = pvi_network_ports_matrix_driver_link_enable,
  .enable          = pvi_network_ports_matrix_driver_enable,
  .link_status_get =
    pvi_network_ports_matrix_driver_link_status_get,
  .status_get        = pvi_network_ports_matrix_driver_status_get,
  .counter_clear     = pvi_network_ports_matrix_driver_counter_clear,
  .counter_clear_all =
    pvi_network_ports_matrix_driver_counter_clear_all,
  .counter_read      = pvi_network_ports_matrix_driver_counter_read,
  .vlan_tag_mode_set =
    pvi_network_ports_matrix_driver_vlan_tag_mode_set,
  .vlan_tag_mode_get              =
    pvi_network_ports_matrix_driver_vlan_tag_mode_get,
  .link_monitor_set               =
    pvi_network_ports_matrix_driver_link_monitor_set,
  .link_monitor_get               =
    pvi_network_ports_matrix_driver_link_monitor_get,
  .link_monitor_callback_register =
    pvi_network_ports_matrix_driver_link_monitor_callback_register,
  .sampling_enable                =
    pvi_network_ports_matrix_driver_sampling_enable,
  .sampling_port_config_set       =
    pvi_network_ports_matrix_driver_sampling_port_config_set,
  .sampling_port_config_get       =
    pvi_network_ports_matrix_driver_sampling_port_config_get,
  .dma_port_init                  = NULL
};

/* Network ports driver interface to register to the MAC manager */
static const struct pvi_network_ports_mgr_driver_interface
  network_ports_driver_interface =
{
  .name = NETWORK_PORTS_DRIVER_NAME,
  .arch = PVI_ARCH_BTTF_PACUARE,
  .ops  = &network_ports_driver_ops
};

static int pvi_network_ports_matrix_driver_probe(
  struct  pvi_driver_instance *driver_instance)
{
  int status = 0;

  // Check if the driver_instance is NULL
  if (NULL == driver_instance) return -EFAULT;

  /* Register to network ports manager */
  status = pvi_network_ports_mgr_driver_register(
    (struct pvi_network_ports_mgr_driver_interface *)
    &network_ports_driver_interface);

  return status;
}

static int pvi_network_ports_matrix_driver_remove(
  struct  pvi_driver_instance *driver_instance)
{
  int status = 0;

  // Check if the driver_instance is NULL
  if (NULL == driver_instance) return -EFAULT;

  /* Remove from network ports manager */
  status = pvi_network_ports_mgr_driver_remove(
    (struct pvi_network_ports_mgr_driver_interface *)
    &network_ports_driver_interface);
  if (status) return status;

  return status;
}

static const struct pvi_driver_instance_id
  pvi_network_ports_matrix_driver_instance_ids[] =
{
  { .compatible = "pvi_matrix_network_ports", .data = NULL },
  {}
};

static struct pvi_driver network_ports_driver =
{
  .name                = NETWORK_PORTS_DRIVER_NAME,
  .driver_instance_ids = pvi_network_ports_matrix_driver_instance_ids,
  .probe               = pvi_network_ports_matrix_driver_probe,
  .remove              = pvi_network_ports_matrix_driver_remove
};

PVI_DRIVER_INIT(network_ports_driver, "pvi_network_ports")

