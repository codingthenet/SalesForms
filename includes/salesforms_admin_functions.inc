<?php
// $Id$

/**
 * @file
 *   Database querying functions used by Salesforms
 */

/**
 * Function to return array of safe fields, either as a nested array of
 * objects and fields or a complete set of field data.
 *
 * @param $param string
 *   If $param === 'list' then only the objects and child fields are returned.
 *   Otherwise all field data is returned.
 *
 * @return $output array
 *   A nested array of [object_name] => [field_name] => [field_data]
 */

function salesforms_get_enabled_fields($param = NULL) {
  $output = array();
  
  // Make sure that any API disabled objects are not enabled in the config
  _salesforms_check_config();
  
  if ($param === 'list') {
    $query = db_query("SELECT object_name, field_name FROM {".SALESFORMS_FIELD_TABLE."}");
    
    while ($fieldmap = db_fetch_array($query)) {
      $output[$fieldmap['object_name']][$fieldmap['field_name']] = $fieldmap['field_name'];
    }
  }
  else {
    $query = db_query("SELECT * FROM {".SALESFORMS_FIELD_TABLE."}");
    
    while ($fieldmap = db_fetch_array($query)) {
      $output[$fieldmap['object_name']][$fieldmap['field_name']] = $fieldmap;
    }
  }
  
  return $output;
}


/**
 * Retrieves fieldmap for a given Drupal form id
 *
 * @param $form_id
 *   String containing the id of the Drupal form, or FALSE
 * 
 * @return $output
 *   If no $form_id, supplies all fieldmaps indexed by form id.
 *   Otherwise array keyed by form id, containing all mapped Drupal form fields 
 *   and their relevant Salesforce objects and fields
 */

function salesforms_get_map($form_id = FALSE) {
  $output = array();
  
  // Update the list of enabled fields as per the API-enabled objects
  _salesforms_check_config();
  
  // JOIN tables - if the fieldmap no longer exists we don't need the fields
  $sql = "SELECT *
          FROM {".SALESFORMS_WEBFORM_MAP_TABLE."} AS t1
          JOIN {".SALESFORMS_FIELD_TABLE."} AS t2
          ON t1.fid = t2.fid";
  
  // If there is no $form_id passed, return all fieldmaps
  if (!$form_id) {
    $query = db_query($sql);
    
    while ($row = db_fetch_array($query)) {
      $output[$row['form_id']][$row['form_field']] = $row;
    }
  }
  // If there is a $form_id, try and return the relevant fieldmap
  else {
    $query = db_query($sql . " WHERE form_id = '%s'", $form_id);
    
    while ($row = db_fetch_array($query)) {
      $output[$row['form_field']] = $row;
    }
  }
  
  return $output;
}


/**
 * Retrieves an array of all Drupal forms with Salesforce maps
 * 
 * @return $output
 *   An array of form ids indexed by form id
 */

function salesforms_get_forms() {
  $output = array();
  // @TODO
  //   Why is this distinct - should only be able to have a single map per form
  $query = db_query("SELECT DISTINCT form_id FROM {".SALESFORMS_WEBFORM_MAP_TABLE."}");
  
  while ($form_id = db_result($query)) {
    $output[$form_id] = $form_id;
  }
  
  return $output;
}