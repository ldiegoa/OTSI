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

// Author: Armando Sandoval (armando.sandoval@hpe.com)

// External
#include <provision/pv_vports.h>
#include <provision/pv_profile.h>
#include <provision/pv_network_ports.h>

// #include
// <provision/internal/ucx/counters_subsystem/pvi_ucx_counters_mac_utils.h>

// Global test headers
#include <pvtest/pvtest.hpp>

// Test-specific headers
#include <provision/pv.h>

// Maximum node ID
#define MAX_NODE_ID 12

// Maximum valid port ID
#define MAX_PORT_ID 58

// Maximum supported index of speed enum (40G)
#define MAX_SPEED_IDX 7

// Minimum supported index of speed enum (10M)
#define MIN_SPEED_IDX 0

// Maximum valid index of fc mode enum
#define MAX_FC_IDX 7

// Maximum size accepted for a frame
#define MAX_FRAME_SIZE 9600

// Max number of iterations for a test sequence
#define MAX_ITER_TEST 3

/** First recirculation port_id in standard mode */
#define FIRST_RECIRCULATION_PORT_ID 2

/** Number of front plane ports */
#define FRONT_PLANE_PORTS 60

/** Max number of front plane ports */
#define MAX_FRONT_PLANE_PORTS 57

/** Min number of front plane ports */
#define MIN_FRONT_PLANE_PORTS 2

/** Supports Conduit/OOBM Port (C), DMA Port (D), 1 Gb/s Port (G)  */
#define SUPPORTED_SPEEDS_FPG_C_D_G PV_PORT_SPEED_10M | PV_PORT_SPEED_100M | \
  PV_PORT_SPEED_1G

/** Supports Recirculation Port (R) */
#define SUPPORTED_SPEEDS_FPG_R     PV_PORT_SPEED_MAX

/** Supports 40 Gb/s Port (XL) */
#define SUPPORTED_SPEEDS_FPG_XL    PV_PORT_SPEED_40G

/** Supports 10 Gb/s Port (X)  */
#define SUPPORTED_SPEEDS_FPG_X     PV_PORT_SPEED_10M | PV_PORT_SPEED_100M | \
  PV_PORT_SPEED_1G | PV_PORT_SPEED_100G

/** Supports Recirculation Port (R), 40 Gb/s Port (XL) */
#define SUPPORTED_SPEEDS_FPG_R_XL  PV_PORT_SPEED_MAX | PV_PORT_SPEED_10M | \
  PV_PORT_SPEED_100M | PV_PORT_SPEED_1G | PV_PORT_SPEED_100G

/** Fast Connect Port (FC), 1 Gb/s Port (G)  */
#define SUPPORTED_SPEEDS_FPG_FC_G  PV_PORT_SPEED_10M | PV_PORT_SPEED_100M | \
  PV_PORT_SPEED_1G |   PV_PORT_SPEED_2P5G | PV_PORT_SPEED_5G | PV_PORT_SPEED_10G

// C O N S T A N T S

/**
 * Supported speeds per port. It's used to make sure that a port supports a
 * specific speed before setting it.
 */
static const uint32_t available_speed_masks[FRONT_PLANE_PORTS] = {
  SUPPORTED_SPEEDS_FPG_C_D_G, /* Port 0 */
  SUPPORTED_SPEEDS_FPG_C_D_G, /* Port 1 */
  SUPPORTED_SPEEDS_FPG_R_XL,  /* Port 2 */
  SUPPORTED_SPEEDS_FPG_R,     /* Port 3 */
  SUPPORTED_SPEEDS_FPG_R,     /* Port 4 */
  SUPPORTED_SPEEDS_FPG_R,     /* Port 5 */
  SUPPORTED_SPEEDS_FPG_X,     /* Port 6 */
  SUPPORTED_SPEEDS_FPG_X,     /* Port 7 */
  SUPPORTED_SPEEDS_FPG_X,     /* Port 8 */
  SUPPORTED_SPEEDS_FPG_X,     /* Port 9 */
  SUPPORTED_SPEEDS_FPG_FC_G,  /* Port 10 */
  SUPPORTED_SPEEDS_FPG_FC_G,  /* Port 11 */
  SUPPORTED_SPEEDS_FPG_FC_G,  /* Port 12 */
  SUPPORTED_SPEEDS_FPG_FC_G,  /* Port 13 */
  SUPPORTED_SPEEDS_FPG_FC_G,  /* Port 14 */
  SUPPORTED_SPEEDS_FPG_FC_G,  /* Port 15 */
  SUPPORTED_SPEEDS_FPG_FC_G,  /* Port 16 */
  SUPPORTED_SPEEDS_FPG_FC_G,  /* Port 17 */
  SUPPORTED_SPEEDS_FPG_C_D_G, /* Port 18 */
  SUPPORTED_SPEEDS_FPG_C_D_G, /* Port 19 */
  SUPPORTED_SPEEDS_FPG_C_D_G, /* Port 20 */
  SUPPORTED_SPEEDS_FPG_C_D_G, /* Port 21 */
  SUPPORTED_SPEEDS_FPG_C_D_G, /* Port 22 */
  SUPPORTED_SPEEDS_FPG_C_D_G, /* Port 23 */
  SUPPORTED_SPEEDS_FPG_C_D_G, /* Port 24 */
  SUPPORTED_SPEEDS_FPG_C_D_G, /* Port 25 */
  SUPPORTED_SPEEDS_FPG_C_D_G, /* Port 26 */
  SUPPORTED_SPEEDS_FPG_C_D_G, /* Port 27 */
  SUPPORTED_SPEEDS_FPG_C_D_G, /* Port 28 */
  SUPPORTED_SPEEDS_FPG_C_D_G, /* Port 29 */
  SUPPORTED_SPEEDS_FPG_C_D_G, /* Port 30 */
  SUPPORTED_SPEEDS_FPG_C_D_G, /* Port 31 */
  SUPPORTED_SPEEDS_FPG_C_D_G, /* Port 32 */
  SUPPORTED_SPEEDS_FPG_C_D_G, /* Port 33 */
  SUPPORTED_SPEEDS_FPG_C_D_G, /* Port 34 */
  SUPPORTED_SPEEDS_FPG_C_D_G, /* Port 35 */
  SUPPORTED_SPEEDS_FPG_C_D_G, /* Port 36 */
  SUPPORTED_SPEEDS_FPG_C_D_G, /* Port 37 */
  SUPPORTED_SPEEDS_FPG_C_D_G, /* Port 38 */
  SUPPORTED_SPEEDS_FPG_C_D_G, /* Port 39 */
  SUPPORTED_SPEEDS_FPG_C_D_G, /* Port 40 */
  SUPPORTED_SPEEDS_FPG_C_D_G, /* Port 41 */
  SUPPORTED_SPEEDS_FPG_C_D_G, /* Port 42 */
  SUPPORTED_SPEEDS_FPG_C_D_G, /* Port 43 */
  SUPPORTED_SPEEDS_FPG_C_D_G, /* Port 44 */
  SUPPORTED_SPEEDS_FPG_C_D_G, /* Port 45 */
  SUPPORTED_SPEEDS_FPG_C_D_G, /* Port 46 */
  SUPPORTED_SPEEDS_FPG_C_D_G, /* Port 47 */
  SUPPORTED_SPEEDS_FPG_C_D_G, /* Port 48 */
  SUPPORTED_SPEEDS_FPG_C_D_G, /* Port 49 */
  SUPPORTED_SPEEDS_FPG_C_D_G, /* Port 50 */
  SUPPORTED_SPEEDS_FPG_C_D_G, /* Port 51 */
  SUPPORTED_SPEEDS_FPG_C_D_G, /* Port 52 */
  SUPPORTED_SPEEDS_FPG_C_D_G, /* Port 53 */
  SUPPORTED_SPEEDS_FPG_C_D_G, /* Port 54 */
  SUPPORTED_SPEEDS_FPG_C_D_G, /* Port 55 */
  SUPPORTED_SPEEDS_FPG_C_D_G, /* Port 56 */
  SUPPORTED_SPEEDS_FPG_C_D_G, /* Port 57 */
  SUPPORTED_SPEEDS_FPG_C_D_G, /* Port 58 */
  SUPPORTED_SPEEDS_FPG_C_D_G  /* Port 59 */
};

// The fixture for testing class
class matrix_pv_network_ports : public ::testing::Test {
public:

  uint32_t chassis_id    = 0;
  uint32_t node_id       = 0;
  uint32_t port_id       = 0;
  struct pv_vport bitmap = {};
  struct pv_vport vport = {};

  enum pv_port_speed speed = PV_PORT_SPEED_MAX;

  enum pv_port_speed rand_speed(uint32_t p_id)
  {
    enum pv_port_speed port_speed = PV_PORT_SPEED_MAX;

    do {
      port_speed = (enum pv_port_speed)(1 << (MIN_SPEED_IDX +
                                              (pvtest_rand.get_rnd() %
                                               (MAX_SPEED_IDX - MIN_SPEED_IDX +
                                                1))));
    } while (0 == (port_speed & available_speed_masks[p_id]));

    return port_speed;
  }

protected:

  matrix_pv_network_ports() {
    // You can do set-up work for each test here.
  }

  virtual ~matrix_pv_network_ports() {
    // You can do clean-up work that doesn't throw exceptions here.
  }

  virtual void SetUp() {
    // Code here will be called immediately after the constructor (right
    // before each test).

    RecordProperty("seed", pvtest_rand.sync_seed());

    int status                        = 0;
    struct pv_asic_config asic_config = {
      .chassis_num = 0,
      .node_num    = 0
    };

    node_id = pvtest_rand.get_rnd() % (MAX_NODE_ID - 1);
    port_id = MIN_FRONT_PLANE_PORTS + pvtest_rand.get_rnd() %
              (MAX_FRONT_PLANE_PORTS - MIN_FRONT_PLANE_PORTS);
    speed = rand_speed(port_id);

    /* Initialize the SDK */
    status = pv_initialize();
    EXPECT_EQ(PV_OK, status);

    /* Attach the node */
    status = pv_attach(node_id,
                       "bttf:///sim",
                       false,
                       "",
                       "pacuare/pacuare_base.yaml");
    EXPECT_EQ(PV_OK, status);

    /* Initialize ASIC */
    status = pv_asic_initialize(node_id, asic_config);
    EXPECT_EQ(0, status);

    /* creates the vports structure */
    status = pv_vports_phys_port_init(chassis_id, node_id, port_id, &vport);
    EXPECT_EQ(0, status);

    /* creates the bitmap structure */
    status = pv_vports_bitmap_init(chassis_id, node_id, &bitmap);
    EXPECT_EQ(PV_OK, status);

    for (uint32_t i = 0; i < port_id; i++) {
      status = pv_vports_phys_port_init(chassis_id,
                                        node_id,
                                        i,
                                        &vport);
      EXPECT_EQ(PV_OK, status);

      status = pv_vports_bitmap_vport_add(vport, &bitmap);
      EXPECT_EQ(PV_OK, status);
    }

    /* Initialize the feature */
    //status = pv_network_ports_initialize(node_id);
    //EXPECT_EQ(PV_OK, status);
  }

  virtual void TearDown() {
    int status = 0;

    /* Uninitialize the feature */
    status = pv_network_ports_uninitialize(node_id);
    EXPECT_EQ(PV_OK, status);

    status = pv_vports_bitmap_vport_remove(vport, &bitmap);
    EXPECT_EQ(PV_OK, status);

    /* Deattach the node */
    status = pv_deattach(node_id);
    EXPECT_EQ(PV_OK, status);

    /* Uninitialize the SDK */
    status = pv_terminate();
    EXPECT_EQ(PV_OK, status);
  }
};

/*
 * Test that verifies network ports initialization
 * Check1 node is not attached
 * Check2 vport structure is not created
 * Check3 profile resources area OUT of boundaries (PORTS_NUM,
 * FRONT_PLANE_PORTS....)
 * Check4 profile resources area IN bounds
 * Check5 FLI is already initialized
 */
TEST_F(matrix_pv_network_ports, pv_network_ports_initialize) {
  int status                        = 0;
  struct pv_vport first_port_failed = {};

  /* Initialize the feature */

  status = pv_network_ports_initialize(node_id);

  EXPECT_EQ(PV_OK, status);

  status = pv_network_ports_init(node_id, bitmap, &first_port_failed);

  EXPECT_EQ(PV_OK, status);
}

/*
 * Test that port speed is successfully configured and retrieved.
 * Check 1: node is not attached
 * Check 2: feature is available and initialized
 * Check 3: Speed set to 10M
 * Check 4: Speed get retrieves 10M
 * Check 5: Speed set to 100M
 * Check 6: Speed get retrieves 100M
 */
TEST_F(matrix_pv_network_ports, network_ports_speed_set_get) {
  int status                  = 0;
  struct pv_vport first_port_failed = {};

  status = pv_network_ports_initialize(node_id);

  EXPECT_EQ(PV_OK, status);

  status = pv_network_ports_init(node_id, bitmap, &first_port_failed);

  EXPECT_EQ(PV_OK, status);


  /*status = pv_network_ports_uninitialize(node_id);
  EXPECT_EQ(PV_OK, status);
  status = pv_network_ports_speed_set(node_id,
                                      vport,
                                      PV_PORT_SPEED_10M,
                                      &port_failed);
  EXPECT_EQ(0, status);
  */
  status = pv_network_ports_speed_set(node_id,
                                      bitmap,
                                      PV_PORT_SPEED_10M,
                                      &first_port_failed);
  EXPECT_EQ(0,     status);
  /*status = pv_initialize();
  EXPECT_EQ(PV_OK, status);


  EXPECT_TRUE(true);*/
}

TEST_F(matrix_pv_network_ports, pv_network_ports_status_get) {
  int status                        = 0;
  struct pv_vport first_port_failed = {};
  struct pv_network_ports_status port_status = {};


  EXPECT_EQ(PV_OK, status);

  status = pv_network_ports_init(node_id, bitmap, &first_port_failed);

  EXPECT_EQ(PV_OK, status);




  status = pv_network_ports_status_get(node_id,
                                       vport,
                                       &port_status);

  status = pv_network_ports_speed_set(node_id,
                                      bitmap,
                                      PV_PORT_SPEED_10M,
                                      &first_port_failed);
  EXPECT_EQ(PV_OK,     status);


  status = pv_network_ports_status_get(node_id,
                                       vport,
                                       &port_status);

  EXPECT_EQ(PV_OK, status);
}




