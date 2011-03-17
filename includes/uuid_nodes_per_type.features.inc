<?php
/**
 * @file
 * Features hooks for the uuid_nodes_per_type features component.
 */

/**
 * Implementation of hook_features_export_options().
 */
function uuid_nodes_per_type_features_export_options() {
  $options = array();

  $types = node_get_types();

  if (is_array($types) && !empty($types)) {
  
    foreach  ($types as $type) {
      $options[$type->type] = t('@type', array(
        '@type' => $type->name, 
      ));
    }
  }

  return $options;
}

/**
 * Implementation of hook_features_export().
 */
function uuid_nodes_per_type_features_export($data, &$export, $module_name = '') {
  $pipe = array();

  $export['dependencies']['uuid_features'] = 'uuid_features';

  uuid_features_load_module_includes();

  foreach ($data as $type) {
    $nodes = _uuid_nodes_per_type_get_nodes(array($type));

    foreach ($nodes as $node) {
      $node = node_load($node->nid);
      $export['features']['uuid_nodes_per_type'][$node->type] = $node->type;
      $export['features']['uuid_node'][$node->uuid] = $node->uuid;
      $pipe['node'][$node->type] = $node->type;

      // drupal_alter() normally supports just one byref parameter. Using
      // the __drupal_alter_by_ref key, we can store any additional parameters
      // that need to be altered, and they'll be split out into additional params
      // for the hook_*_alter() implementations.  The hook_alter signature is
      // hook_uuid_node_features_export_alter(&$export, &$pipe, $node)
      $data = &$export;
      $data['__drupal_alter_by_ref'] = array(&$pipe);
      drupal_alter('uuid_node_features_export', $data, $node);
    }
  }

  return $pipe;
}

/**
 * Implementation of hook_features_export_render().
 */
function uuid_nodes_per_type_features_export_render($module, $data) {
  $translatables = $code = array();
  uuid_features_load_module_includes();

  $code[] = '  $nodes = array();';
  $code[] = '';
  foreach ($data as $type) {
    $rows = _uuid_nodes_per_type_get_nodes($type);

    foreach ($rows as $row) {
       $code[] = '  $nodes[] = \''. $row->uuid .'\';';
    }
  }

  $code[] = '  return $nodes;';
  $code = implode("\n", $code);

  watchdog('features_render', var_export($code, TRUE));

  return array('uuid_features_default_content_per_type' => $code);
}

/**
 * Implementation of hook_features_revert().
 */
function uuid_nodes_per_type_features_revert($module) {
  uuid_nodes_per_type_features_rebuild($module);
}

/**
 * Implementation of hook_features_rebuild().
 * Rebuilds nodes based on UUID from code defaults.
 */
function uuid_nodes_per_type_features_rebuild($module) {
  $module_info = feature_load($module);
  $types = $module_info->info['features']['uuid_nodes_per_type'];

  if (is_array($types) && !empty($types)) {
    $nodes_default = module_invoke($module, 'uuid_features_default_content_per_type');
    $nodes = _uuid_nodes_per_type_get_nodes($types);
    
    if (!empty($nodes)) {

      foreach ($nodes as $node) {
        
        if (!in_array($node->uuid, $nodes_default)) {
          node_delete($node->nid);
        }
      }
    }
  }
}

/**
 * Get all nodes from a specific content_type
 */
function _uuid_nodes_per_type_get_nodes($types) {
  $rows = array();
  $res = db_query(
    "SELECT n.nid, n.title, n.type, u.uuid FROM {node} n 
    INNER JOIN {uuid_node} u ON n.nid = u.nid WHERE n.type IN (" . db_placeholders($types, 'varchar') .")", $types
  );

  while ($row = db_fetch_object($res)) {
    $rows[] = $row;
  }

  return $rows;
}
