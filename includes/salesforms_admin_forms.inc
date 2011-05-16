<?php
// $Id$

/**
 * @file
 *   Administration forms for the Salesforms tab in the Salesforce menu
 */

/**
 * Administration form for security-enabled fields. Generates named object
 * fieldsets containing a collapsed fieldset of field tickboxes.
 */

function salesforms_enabled_fields_form(&$form_state) {
  
  $form = array();
  
  // Retrieve the Salesforce API cached config
  $api_enabled_objects = salesforce_api_fieldmap_objects();
  
  // If the config returns an empty array, there are no objects enabled
  if (empty($api_enabled_objects)) {
    drupal_set_message(t('You have not enabled any objects'), 'error');
    return $form;
  }
  
  // Otherwise something is enabled, so retrieve the main Salesforms config
  $salesforms_config = salesforms_get_config();
  
  // Then generate the form used to manage objects at SALESFORCE_PATH_ADMIN . '/security'.
  foreach ($salesforms_config as $obj_name => $obj_data) {
    // Create a fieldset for the object
    $form['submitted['.$obj_name.']'] = array(
      '#type' => 'fieldset',
      '#title' => '[' . $obj_name . '] ' . $obj_data->label,
      '#collapsible' => TRUE,
      '#collapsed' => TRUE,
      '#tree' => TRUE,
    );
    
    // And create a checkbox for each field
    foreach ( $obj_data->fields as $field_data ) {
      // @TODO 
      //   Check $field_data->defaultedOnCreate vs. reference fields
      //   Also the label logic is a bit clunky
      if ( $field_data->createable ) {
        $field_label = $field_data->label;
        
        // If it's a reference field, mark the label
        if ( $field_data->referenceTo ) {
          $field_label .= ' reference to ' . $field_data->referenceTo;
        }
        
        $form['submitted['.$obj_name.']'][$field_data->name] = array(
          '#type' => 'checkbox',
          '#title' => 'Enable ' . $field_label,
          '#return_value' => $field_data->name,
          '#default_value' => isset($field_data->salesformsEnabled),
        );
      }
    }
  }
  
  $form['submit'] = array(
    '#type' => 'submit',
    '#value' => 'Save'
  );
  
  return $form;
}


/**
 * Returns all reference fields from Salesforce given an array of SF objects
 *
 * @param $object_array array
 *   Array of SalesForce objects
 * 
 * @return $reference_fields array
 *   Array of reference fields by object
 */

function salesforms_get_reference_fields($object_array) {
  $object_data = salesforce_api_describeSObjects($object_array);
  
  foreach ( $object_data as $object_name => $object_data ) {
    foreach ( $object_data->fields as $field_data ) {
      if ( $field_data->referenceTo && $field_data->createable && !$field_data->defaultedOnCreate) {
        $reference_fields[$object_name][$field_data->name] = array( 'field_name' => $field_data->name, 'reference_to' => $field_data->referenceTo );
      }
    }
  }
  
  return $reference_fields;
}


/**
 * Field administration form submit handler - pushes field data into external database.
 */

function salesforms_enabled_fields_form_submit($form, &$form_state) {
  // Separate the useful data from the form POST
  $post = $form['#post']['submitted'];
  
  // Retrieve the main Salesforms config
  $salesforms_config = salesforms_get_config();
  
  // Create the arrays for the query variables
  $insert_values = $delete_values = $insert_placeholders = $delete_placeholders = array();
  
  // Iterate through all fields in the config to find which ones to update
  foreach ($salesforms_config as $object_name => $object_data) {
    foreach($object_data->fields as $index => $field_data) {
      // If the field is enabled in the POST but not in the config, insert
      if (isset($post[$object_name][$field_data->name]) && !isset($field_data->salesformsEnabled)) {
        $insert_values[] = $object_name;
        $insert_values[] = $field_data->name;
        $insert_values[] = $field_data->referenceTo;
        $insert_placeholders[] = "('%s', '%s', '%s')";
      }
      // If the field is enabled in the config but not in the POST, delete
      elseif (isset($field_data->salesformsEnabled) && !isset($post[$object_name][$field_data->name])) {
        $delete_values[] = $object_name;
        $delete_values[] = $field_data->name;
        $delete_placeholders[] = "object_name = '%s' AND field_name = '%s'";
      }
    }
  }
  
  if (!empty($insert_values)) {
    $insert_sql = "INSERT INTO {salesforms_safe_fields} (object_name, field_name, reference_to) VALUES ";
    db_query($insert_sql . implode(', ', $insert_placeholders), $insert_values);
  }
  
  if (!empty($delete_values)) {
    // @TODO
    //   Could this also be done with cases? Maybe a bit neater?
    $delete_sql = "DELETE FROM {salesforms_safe_fields} WHERE ";
    db_query($delete_sql . implode(' OR ', $delete_placeholders), $delete_values);
  }
  
  return drupal_set_message(t('Your field selection has been updated'));
}


/**
 * Administration form for security-enabled objects. Generates a list of tickboxes
 * for objects that can be created, updated, and deleted.
 */

function salesforms_mapper(&$form_state) {
  $form = array();
  $fieldmaps = salesforms_get_enabled_fields();
  
  // If there are no enabled objects, don't bother printing the form
  if (empty($fieldmaps)) {
    drupal_set_message( t( 'You have not enabled any SalesForce fields' ), 'error' );
    return $form;
  }
  
  foreach ($fieldmaps as $object_name => $fields) {
    $fieldset_name = 'submitted[' . $object_name . ']';
    
    $form[$fieldset_name] = array(
      '#type' => 'fieldset',
      '#title' => $object_name,
      '#collapsible' => FALSE,
      '#tree' => TRUE,
    );
    
    foreach ($fields as $field_name => $field_data) {
      // If required is true or the default_val is set, the fieldset is not collapsed
      $form[$fieldset_name][$field_name] = array(
        '#type' => 'fieldset',
        '#title' => $field_name,
        '#collapsible' => TRUE,
        '#collapsed' => !( $field_data['required'] || !empty($field_data['default_val']) ),
        '#tree' => TRUE,
      );
      
      $form[$fieldset_name][$field_name]['required'] = array(
        '#type' => 'checkbox',
        '#title' => 'Required',
        '#default_value' => $field_data['required'],
      );
      
      $form[$fieldset_name][$field_name]['default_val'] = array(
        '#type' => 'textfield',
        '#title' => 'Default Value',
        '#maxlength' => 100,
        '#default_value' => $field_data['default_val'],
      );
    }
  }
  
  $form['submit'] = array(
    '#type' => 'submit',
    '#value' => 'Save'
  );
  
  return $form;
}

/**
 * Further settings for enabled objects - can set required or a default value.
 *
 * @ TODO
 *   Explore hinting using best-guess for field requirements from Salesforce, and maybe automatically
 *   set some standard ones like Contact->LastName
 *   
 *   Reduce the for loop queries to single database queries. This is hard to do with two lookup keys...maybe need unique keys
 */

function salesforms_mapper_submit( $form, &$form_state ) {
  $post = $form['#post']['submitted'];
  $sql = "UPDATE {salesforms_safe_fields} SET required = %b, default_val = '%s' WHERE object_name = '%s' AND field_name = '%s'";
  
  foreach ( $post as $object_name => $fields ) {
    foreach ($fields as $field_name => $field_data) {
      $required = isset($field_data['required']) ? 1 : 0;
      $default_val = trim( $field_data['default_val'] );
      
      if ($required || !empty($default_val)){
        db_query( $sql, $required, $default_val, $object_name, $field_name );
      }
    }
  }
}