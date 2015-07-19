<?php

/**
 * @file
 * LaTeX related functions.
 */

/**
 * Generate template.
 *
 * Example:
 * "@@@PREFIX|PLACEHOLDER|SUFFIX@@@" in text will be replaced with "PREFIX|VALUE|SUFFIX"
 * See how "@@@\item |outline|@@@" is used in bp.tex.
 */
function latex_generate_generate($variables) {
  // Get the template.
  $filename = drupal_get_path('module', 'latex_generate') . '/assets/bp.tex';
  $handle = fopen($filename, "r");
  $result = fread($handle, filesize($filename));
  fclose($handle);

  // Handle multiline items.
  $variables = latex_generate_break_lines($variables, 'outline');
  $variables = latex_generate_break_lines($variables, 'literature');

  $patterns = array();
  $replacements = array();
  foreach ($variables as $key => $value) {
    if (is_array($value)) {
      $replacement_temp = '';
      foreach ($value as $value_key => $value_item) {
        $replacement_temp .= '@@@${1}|' . $key . $value_key . '|${2}@@@' . "\n";
      }
      $patterns[] = '/\@\@\@(.*)\|' . $key . '\|(.*)\@\@\@/im';
      $replacements[] = $replacement_temp;

      foreach ($value as $value_key => $value_item) {
        $patterns[] = '/\@\@\@(.*)\|' . $key . $value_key . '\|(.*)\@\@\@/im';
        $replacements[] = '${1}' . $value_item . '${2}';
      }
    }
    else {
      $patterns[] = '/\@\@\@(.*)\|' . $key . '\|(.*)\@\@\@/im';
      $replacements[] = '${1}' . $value . '${2}';
    }
  }

  $result = preg_replace($patterns, $replacements, $result);

  return $result;
}

/**
 * Convert line breaks to array.
 */
function latex_generate_break_lines($variables, $key) {
  $tmp = explode("\n", $variables[$key]);
  $variables[$key] = array();

  foreach ($tmp as $tmp_item) {
    $variables[$key][] = $tmp_item;
  }

  return $variables;
}

/**
 * Handles the .tex file.
 */
function latex_generate_input_handle_tex($filename, $data) {
  $filename = $filename . '.tex';
  $path = 'public://generated/';
  $uri = $path . $filename;

  // Make sure target dir exists.
  file_prepare_directory($path, FILE_CREATE_DIRECTORY);

  // Save data.
  $file = file_save_data($data, $uri, FILE_EXISTS_RENAME);

  // Delete files older then DRUPAL_MAXIMUM_TEMP_FILE_AGE (6 hours).
  $file->status = 0;
  file_save($file);

  if (empty($file)) {
    drupal_set_message(t('Failed to generate file.'), 'error');
    return NULL;
  }

  return $file;
}

/**
 * Handles the .pdf file.
 */
function latex_generate_input_handle_pdf($filename, $data) {
  $path = 'public://generated/';
  $uri = $path . $filename;

  // Make sure target dir exists.
  file_prepare_directory($path, FILE_CREATE_DIRECTORY);
  // Save tex file.
  $file = latex_generate_input_handle_tex($filename, $data);

  // Can we do shell_exec?
  if (!(is_callable('shell_exec') && stripos(ini_get('disable_functions'), 'shell_exec') === FALSE)) {
    // Fallback to .tex.
    return $file;
  }

  // Generate PDF.
  $result = shell_exec('pdflatex ' . drupal_realpath($file->uri));

  // Check PDF.
  if (!(strpos($result, 'Output written on ' . $filename . '.pdf') != FALSE)) {
    // Something went wrong. Fallback to .tex.
    return $file;
  }

  // URI of generated PDF.
  $pdf_uri = $uri . '.pdf';

  // Add generated pdf asn managed file for garbage collection.
  $handle = fopen(drupal_realpath($pdf_uri), 'r');
  $file = file_save_data($handle, $pdf_uri, FILE_EXISTS_REPLACE);
  fclose($handle);
  // Delete files older then DRUPAL_MAXIMUM_TEMP_FILE_AGE (6 hours).
  $file->status = 0;
  file_save($file);

  return $file;
}
